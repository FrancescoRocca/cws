#include "common.h"
#include <stdio.h>

static int test_get_index_ok(void) {
	char url[256];
	int failures = 0;

	snprintf(url, sizeof(url), "%s/", BASE_URL);
	failures += assert_status("GET / returns 200", url, "GET", 200);

	snprintf(url, sizeof(url), "%s/index.html", BASE_URL);
	failures += assert_status("GET /index.html returns 200", url, "GET", 200);

	snprintf(url, sizeof(url), "%s/?v=1", BASE_URL);
	failures += assert_status("GET /?v=1 returns 200", url, "GET", 200);

	snprintf(url, sizeof(url), "%s/", BASE_URL);
	failures += assert_status("HEAD / returns 200", url, "HEAD", 200);

	return failures;
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

	int failures = test_get_index_ok();

	stop_server(server_pid);
	curl_global_cleanup();

	if (failures == 0) {
		fprintf(stdout, "[PASS] test_get_index passed\n");
		return 0;
	}

	fprintf(stderr, "[FAIL] test_get_index failed\n");
	return 1;
}
