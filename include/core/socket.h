#ifndef CWS_SOCKET_H
#define CWS_SOCKET_H

#include <myclib/mystring.h>
#include <sys/types.h>

size_t cws_socket_read(int sockfd, string_s *str);

size_t cws_socket_send(int sockfd, char *buffer, size_t len, int flags);

#endif
