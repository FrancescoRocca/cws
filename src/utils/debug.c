#include "utils/debug.h"
#include <stdarg.h>
#include <stdio.h>
#include <syslog.h>

void cws_log_init(void) {
	openlog("cws", LOG_PID | LOG_CONS, LOG_DAEMON);
}

void _cws_log_info_internal(const char *file, int line, const char *fmt, ...) {
	va_list args;
	fprintf(stdout, _INFO " [%s:%d] ", file, line);
	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
	fprintf(stdout, "\n");

	va_start(args, fmt);
	syslog(LOG_INFO, fmt, args);
	va_end(args);
}

void _cws_log_warning_internal(const char *file, int line, const char *fmt, ...) {
	va_list args;
	fprintf(stdout, _WARNING " [%s:%d] ", file, line);
	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
	fprintf(stdout, "\n");

	va_start(args, fmt);
	syslog(LOG_WARNING, fmt, args);
	va_end(args);
}

void _cws_log_error_internal(const char *file, int line, const char *fmt, ...) {
	va_list args;
	fprintf(stdout, _ERR " [%s:%d] ", file, line);
	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
	fprintf(stdout, "\n");

	va_start(args, fmt);
	syslog(LOG_ERR, fmt, args);
	va_end(args);
}

#ifdef EVELOPER
void _cws_log_debug_internal(const char *file, int line, const char *fmt, ...) {
	va_list args;
	fprintf(stdout, _DEBUG " [%s:%d] ", file, line);
	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);
	fprintf(stdout, "\n");

	va_start(args, fmt);
	syslog(LOG_DEBUG, fmt, args);
	va_end(args);
}
#else
void _cws_log_debug_internal(const char *file, int line, const char *fmt, ...) {
	(void)file;
	(void)line;
	(void)fmt;
	/* Nothing */
}
#endif

void cws_log_shutdown(void) {
	closelog();
}
