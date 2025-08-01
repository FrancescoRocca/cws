#ifndef CWS_COLORS_H
#define CWS_COLORS_H

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

#ifdef EVELOPER
#define CWS_LOG_DEBUG(msg, ...) fprintf(stdout, _DEBUG " [%s:%d] " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define CWS_LOG_DEBUG(msg, ...)
#endif

#define CWS_LOG_ERROR(msg, ...) fprintf(stderr, _ERR " " msg "\n", ##__VA_ARGS__)
#define CWS_LOG_WARNING(msg, ...) fprintf(stdout, _WARNING " " msg "\n", ##__VA_ARGS__)
#define CWS_LOG_INFO(msg, ...) fprintf(stdout, _INFO " " msg "\n", ##__VA_ARGS__)

#endif