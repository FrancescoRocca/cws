#ifndef CWS_SOCKET_H
#define CWS_SOCKET_H

#include <myclib/mystring.h>
#include <sys/types.h>

ssize_t cws_socket_read(int sockfd, string_s *str);

ssize_t cws_socket_send(int sockfd, const char *buffer, size_t len, int flags);

#endif
