#include "utils/debug.h"
#include <stdarg.h>
#include <syslog.h>

void cws_log_init(void) {
	openlog("cws", LOG_PID | LOG_CONS, LOG_DAEMON);
}

void cws_log_info(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	syslog(LOG_INFO, fmt, args);

	va_end(args);
}

void cws_log_error(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	syslog(LOG_ERR, fmt, args);

	va_end(args);
}

void cws_log_shutdown(void) {
	closelog();
}
