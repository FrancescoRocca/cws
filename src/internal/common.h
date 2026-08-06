#ifndef CWS_INTERNALS_COMMON_H
#define CWS_INTERNALS_COMMON_H

#define CONTENT_TYPE_MAX 64
#define HEADER_KEY_MAX 256
#define HEADER_VALUE_MAX 1024
#define CHUNK_SIZE 8192
#define HEADERS_BUFFER_SIZE 2048

/* Maximum request size (headers included) accepted per connection */
#define MAX_REQUEST_SIZE (64 * 1024)

/* Poll timeout (ms) while waiting for the socket to become writable */
#define SOCKET_SEND_POLL_TIMEOUT_MS 5000

#endif
