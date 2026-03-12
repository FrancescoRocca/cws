#include <curl/curl.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define URL "http://localhost:3030"

int main(int argc, char **argv) {
	if (argc < 2) {
		return 1;
	}

	char *server_path = argv[1];
	fprintf(stdout, "server path: %s\n", server_path);

	/* Run the server as child proc */
	pid_t pid = fork();
	if (pid == 0) {
		execl(server_path, "cws", NULL);
		fprintf(stderr, "execl failed: %s\n", strerror(errno));
		exit(1);
	}

	if (pid == -1) {
		fprintf(stderr, "fork failed: %s\n", strerror(errno));
		exit(1);
	}

	fprintf(stdout, "pid: %d\n", pid);

	CURL *curl = curl_easy_init();
	curl_easy_setopt(curl, CURLOPT_URL, URL);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 1L);

	long http_code = 0;
	int max_retries = 10;
	for (int i = 0; i < max_retries; i++) {
		CURLcode res = curl_easy_perform(curl);
		if (res == CURLE_OK) {
			curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
			break;
		}
		fprintf(stdout, "Server not ready, retry %d/%d...\n", i + 1, max_retries);
		sleep(1);
	}

	curl_easy_cleanup(curl);

	fprintf(stdout, "http_code: %ld\n", http_code);

	/* Kill the server */
	kill(pid, SIGTERM);
	waitpid(pid, NULL, 0);

	return http_code == 200 ? 0 : 1;
}
