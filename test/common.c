#include "common.h"
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

bool start_server(const char *server_path, pid_t *pid_out) {
	pid_t pid = fork();
	if (pid == 0) {
		execl(server_path, "cws", NULL);
		fprintf(stderr, "execl failed: %s\n", strerror(errno));
		_exit(1);
	}

	if (pid < 0) {
		fprintf(stderr, "fork failed: %s\n", strerror(errno));
		return false;
	}

	*pid_out = pid;
	fprintf(stdout, "[INFO] server pid: %d\n", pid);
	return true;
}

void stop_server(pid_t pid) {
	if (pid <= 0) {
		return;
	}

	kill(pid, SIGTERM);
	waitpid(pid, NULL, 0);
	fprintf(stdout, "[INFO] server stopped (pid=%d)\n", pid);
}

bool perform_request(const char *url, const char *method, http_result_s *out) {
	CURL *curl = curl_easy_init();
	if (!curl) {
		fprintf(stderr, "[ERROR] curl_easy_init failed\n");
		return false;
	}

	memset(out, 0, sizeof(*out));
	out->content_length = -1;
	out->error[0] = '\0';

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method);
	curl_easy_setopt(curl, CURLOPT_NOBODY, 0L);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 3L);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, out->error);

	out->curl_code = curl_easy_perform(curl);
	if (out->curl_code == CURLE_OK) {
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &out->code);
		curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &out->content_length);

		char *ct = NULL;
		curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct);
		if (ct) {
			strncpy(out->content_type, ct, sizeof(out->content_type) - 1);
			out->content_type[sizeof(out->content_type) - 1] = '\0';
		}
	}

	curl_easy_cleanup(curl);
	return out->curl_code == CURLE_OK;
}

bool wait_until_ready(void) {
	char url[256];
	snprintf(url, sizeof(url), "%s/", BASE_URL);

	for (int i = 0; i < MAX_RETRIES; i++) {
		http_result_s result;
		if (perform_request(url, "GET", &result)) {
			fprintf(stdout, "[INFO] server ready after %d attempt(s), code=%ld\n", i + 1, result.code);
			return true;
		}

		fprintf(stdout, "[INFO] server not ready (%d/%d): %s\n", i + 1, MAX_RETRIES,
				result.error[0] ? result.error : "unknown error");
		sleep(RETRY_SLEEP_SEC);
	}

	return false;
}

int assert_status(const char *name, const char *url, const char *method, long expected) {
	http_result_s result;
	if (!perform_request(url, method, &result)) {
		fprintf(stderr, "[FAIL] %s: request failed (%s)\n", name,
				result.error[0] ? result.error : "unknown curl error");
		return 1;
	}

	fprintf(stdout, "[TEST] %s -> %s %s => %ld (expected %ld)\n", name, method, url, result.code, expected);

	if (result.code != expected) {
		fprintf(stderr, "[FAIL] %s: expected HTTP %ld, got %ld\n", name, expected, result.code);
		return 1;
	}

	return 0;
}
