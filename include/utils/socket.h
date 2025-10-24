#ifndef CWS_SOCKET_H
#define CWS_SOCKET_H

#include <myclib/mystring.h>
#include <sys/types.h>

ssize_t cws_read_data(int sockfd, string_s *str);
ssize_t cws_send_data(int sockfd, char *buffer, int len, int flags);

#endif
