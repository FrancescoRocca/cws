#ifndef CWS_TEST_COMMON_H
#define CWS_TEST_COMMON_H

#include <curl/curl.h>
#include <stdbool.h>
#include <sys/types.h>

#define BASE_URL "http://localhost:3030"
#define MAX_RETRIES 15
#define RETRY_SLEEP_SEC 1

typedef struct http_result {
	long code;
	long content_length;
	char content_type[256];
	char error[CURL_ERROR_SIZE];
	CURLcode curl_code;
} http_result_s;

bool start_server(const char *server_path, pid_t *pid_out);
void stop_server(pid_t pid);
bool wait_until_ready(void);
bool perform_request(const char *url, const char *method, http_result_s *out);
int assert_status(const char *name, const char *url, const char *method, long expected);

#endif
