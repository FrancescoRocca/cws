#ifndef CWS_DEBUG_H
#define CWS_DEBUG_H

/* ANSI color escape sequences */
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define BOLD "\033[1m"
#define RESET "\033[0m"

#ifdef USE_COLORS
#define _ERR RED BOLD "[ERROR]" RESET
#define _WARNING YELLOW "[WARNING]" RESET
#define _INFO GREEN "[INFO]" RESET
#define _DEBUG BLUE "[DEBUG]" RESET
#else
#define _ERR "[ERROR]"
#define _WARNING "[WARNING]"
#define _INFO "[INFO]"
#define _DEBUG "[DEBUG]"
#endif

void cws_log_init(void);
void cws_log_info(const char *fmt, ...);
void cws_log_debug(const char *fmt, ...);
void cws_log_warning(const char *fmt, ...);
void cws_log_error(const char *fmt, ...);
void cws_log_shutdown(void);

#endif
