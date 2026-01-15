#include "utils/debug.h"
#include <stdarg.h>
#include <stdio.h>
#include <sys/syslog.h>

void cws_log_init(void) {
	openlog("cws", LOG_PID | LOG_CONS, LOG_DAEMON);
}

void cws_log_info(const char *fmt, ...) {
	va_list args;
	fprintf(stdout, _INFO " ");
	va_start(args, fmt);

	vfprintf(stdout, fmt, args);
	syslog(LOG_INFO, fmt, args);

	va_end(args);
	fprintf(stdout, "\n");
}

void cws_log_warning(const char *fmt, ...) {
	va_list args;
	fprintf(stdout, _WARNING " ");
	va_start(args, fmt);

	vfprintf(stdout, fmt, args);

	va_end(args);
	fprintf(stdout, "\n");
}

void cws_log_error(const char *fmt, ...) {
	va_list args;
	fprintf(stdout, _ERR " ");
	va_start(args, fmt);

	vfprintf(stdout, fmt, args);
	syslog(LOG_ERR, fmt, args);

	va_end(args);
	fprintf(stdout, "\n");
}

void cws_log_shutdown(void) {
	closelog();
}

#ifdef EVELOPER
void cws_log_debug(const char *fmt, ...) {
	fprintf(stdout, _DEBUG " [%s:%d] ", __FILE__, __LINE__);
	va_list args;
	va_start(args, fmt);

	vfprintf(stdout, fmt, args);
	syslog(LOG_DEBUG, fmt, args);

	va_end(args);
	fprintf(stdout, "\n");
}
#else
void cws_log_debug(const char *fmt, ...) {
	/* Nothing */
}
#endif
