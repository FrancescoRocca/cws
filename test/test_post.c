#include "common.h"
#include <stdio.h>

static int test_post_not_implemented(void) {
	char url[256];
	snprintf(url, sizeof(url), "%s/", BASE_URL);
	return assert_status("POST / returns 501", url, "POST", 501);
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

	int failures = test_post_not_implemented();

	stop_server(server_pid);
	curl_global_cleanup();

	if (failures == 0) {
		fprintf(stdout, "[PASS] test_post passed\n");
		return 0;
	}

	fprintf(stderr, "[FAIL] test_post failed\n");
	return 1;
}
