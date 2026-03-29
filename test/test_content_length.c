#include "common.h"
#include <stdio.h>

static int test_content_length_present_on_index(void) {
	char url[256];
	snprintf(url, sizeof(url), "%s/", BASE_URL);

	http_result_s result;
	if (!perform_request(url, "GET", &result)) {
		fprintf(stderr, "[FAIL] content-length on index: request failed (%s)\n",
				result.error[0] ? result.error : "unknown curl error");
		return 1;
	}

	fprintf(stdout, "[TEST] content-length on index -> code=%ld, content-length=%ld\n", result.code,
			result.content_length);

	if (result.code != 200) {
		fprintf(stderr, "[FAIL] content-length on index: expected 200, got %ld\n", result.code);
		return 1;
	}

	if (result.content_length < 0) {
		fprintf(stderr, "[FAIL] content-length on index: missing/invalid content length\n");
		return 1;
	}

	return 0;
}

int main(int argc, char **argv) {
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <server_binary_path>\n", argv[0]);
		return 1;
	}

	curl_global_init(CURL_GLOBAL_DEFAULT);

	const char *server_path = argv[1];
	fprintf(stdout, "[INFO] server path: %s\n", server_path);

	pid_t server_pid = -1;
	if (!start_server(server_path, &server_pid)) {
		curl_global_cleanup();
		return 1;
	}

	if (!wait_until_ready()) {
		fprintf(stderr, "[FAIL] server did not become ready in time\n");
		stop_server(server_pid);
		curl_global_cleanup();
		return 1;
	}

	int failures = test_content_length_present_on_index();

	stop_server(server_pid);
	curl_global_cleanup();

	if (failures == 0) {
		fprintf(stdout, "[PASS] test_content_length passed\n");
		return 0;
	}

	fprintf(stderr, "[FAIL] test_content_length failed\n");
	return 1;
}
