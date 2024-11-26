# Basic Concepts

Before reading, this document could contain errors, please check everything you read.

- [Basic Concepts](#basic-concepts)
    - [Socket](#socket)
    - [Internet sockets](#internet-sockets)
    - [Byte Order](#byte-order)
    - [Structs](#structs)
        - [struct addrinfo](#struct-addrinfo)
        - [struct sockaddr](#struct-sockaddr)
        - [struct sockaddr\_in](#struct-sockaddr_in)
        - [struct sockaddr\_storage](#struct-sockaddr_storage)
    - [IP Addresses](#ip-addresses)
    - [getaddrinfo()](#getaddrinfo)
    - [socket()](#socket-1)
    - [bind()](#bind)
    - [connect()](#connect)
    - [listen()](#listen)
    - [accept()](#accept)
    - [send() and recv()](#send-and-recv)
    - [sendto() and recvfrom()](#sendto-and-recvfrom)
    - [close() and shutdown()](#close-and-shutdown)
    - [getpeername()](#getpeername)
    - [gethostname()](#gethostname)

### Socket

When Unix programs do some I/O they do it reading/writing to a **file descriptor**. A file descriptor is an integer
associated with an open file, it can be anything. To communicate over the internet using a file descriptor we'll make a
call to the `socket()` system routine.

### Internet sockets

There are two types of Internet sockets:

1. Stream Sockets (SOCK_STREAM) - error-free and a realiable two-way communication (TCP)
2. Datagram Sockets (SOCK_DGRAM) - connectionless (UDP)

### Byte Order

- Big-Endian (also called **Network Byte Order**)
- Little-Endian

Before making any transmission we have to convert the byte order to a Network Byte Order, we can do this with simple
functions:

| Function | Description           |
|----------|-----------------------|
| htons()  | Host to Network Short |
| htonl()  | Host to Network Long  |
| ntohs()  | Network to Host Short |
| ntohl()  | Network to Host Long  |

And convert the answer to the host byte order.

### Structs

#### struct addrinfo

This struct prepares the socket address strcutures for subsequent use.

```c
struct addrinfo {
    int              ai_flags;     // AI_PASSIVE, AI_CANONNAME, etc.
    int              ai_family;    // AF_INET, AF_INET6, AF_UNSPEC
    int              ai_socktype;  // SOCK_STREAM, SOCK_DGRAM
    int              ai_protocol;  // use 0 for "any"
    size_t           ai_addrlen;   // size of ai_addr in bytes
    struct sockaddr *ai_addr;      // struct sockaddr_in or _in6
    char            *ai_canonname; // full canonical hostname

    struct addrinfo *ai_next;      // linked list, next node
};
```

#### struct sockaddr

Inside the struct we can see there is a pointer to the `struct sockaddr`, that is defined as follows:

```c
struct sockaddr {
    unsigned short    sa_family;    // address family, AF_xxx
    char              sa_data[14];  // 14 bytes of protocol address
};
```

#### struct sockaddr_in

But, we can avoid to pack manually the stuff inside this struct and use the `struct sockaddr_in` (or
`struct sockaddr_in6` for IPv6) with a fast cast that is made for the Internet:

```c
struct sockaddr_in {
    short int          sin_family;  // Address family, AF_INET
    unsigned short int sin_port;    // Port number
    struct in_addr     sin_addr;    // Internet address
    unsigned char      sin_zero[8]; // Same size as struct sockaddr
};
```

`sin_zero` is used to pad the struct to the length of a sockaddr and it should be set to all zeros (`memset()`).

#### struct sockaddr_storage

This struct is designed to storage both IPv4 and IPv6 structures. Example, when a client is going to connect to your
server you don't know if it is a IPv4 or IPv6 so you use this struct and then cast to what you need (check the
`ss_family` first).

```c
struct sockaddr_storage {
    sa_family_t  ss_family;     // address family

    // all this is padding, implementation specific, ignore it:
    char      __ss_pad1[_SS_PAD1SIZE];
    int64_t   __ss_align;
    char      __ss_pad2[_SS_PAD2SIZE];
};
```

### IP Addresses

Here's a way to convert an IP address string into a struct.

```c
struct sockaddr_in sa;
struct sockaddr_in6 sa6;

inet_pton(AF_INET, "192.168.0.1", &(sa.sin_addr));
inet_pton(AF_INET6, "2001:db8:63b3:1::3490", &(sa6.sin6_addr));
```

There is a very easy function called `inet_pton()`, *pton* stands for **Presentation to network**. If you want to do the
same but from binary to string you have `inet_ntop()`.

### getaddrinfo()

```c
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

int getaddrinfo(const char *node,     // e.g. "www.example.com" or IP
                const char *service,  // e.g. "http" or port number
                const struct addrinfo *hints,
                struct addrinfo **res);
```

The `node` could be a host name or IP address. `service` could be a port number or a service found in `/etc/services` (
e.g. "http" or "ftp" or "telnet").
`hints` points to a struct you already filled and `res` contains a linked list of results.

### socket()

```c
#include <sys/types.h>
#include <sys/socket.h>

int socket(int domain, int type, int protocol); 
```

`domain` could be `PF_INET` or `PF_INET6`, `type` instead TCP or UDP and `protocol` to 0. `PF_INET` is very close to
`AF_INET`! However, we can avoid to put the stuff manually and use the results from `getaddrinfo()`.

### bind()

Do this if you're going to listen on a port. The port is used by the kernel to match an incoming packet to a socket
descriptor.

```c
#include <sys/types.h>
#include <sys/socket.h>

int bind(int sockfd, struct sockaddr *my_addr, int addrlen);
```

### connect()

```c
#include <sys/types.h>
#include <sys/socket.h>

int connect(int sockfd, struct sockaddr *serv_addr, int addrlen); 
```

### listen()

```c
int listen(int sockfd, int backlog);
```

`backlog` is the amount of max clients allowed on the incoming queue. A client will wait in the queue until you accept
it.

### accept()

```c
#include <sys/types.h>
#include <sys/socket.h>

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

After you accept a client, the function will return a new socket file descriptor used to communicate.

### send() and recv()

```c
int send(int sockfd, const void *msg, int len, int flags); 
```

Just put `flags` to 0. It will return the bytes sent, but sometimes it could not match the len of the data sent, it's up
to you to send the rest of the string (it should sent 1K of data without splitting).

```c
int recv(int sockfd, void *buf, int len, int flags);
```

Put 0 at `flags` (see the man page for more info). The `sockfd` is the file descriptor to read from. The function could
return 0 (this means the remote side has closed the connection).

### sendto() and recvfrom()

It's the DGRAM equivalent of STREAM. Marked as *TODO*.

### close() and shutdown()

To close the connection just use the regular Unix file descriptor `close()` function:

```c
close(sockfd);
```

If you want more control over how the socket closes there is `shutdown()`:

```c
int shutdown(int sockfd, int how);
```

`how` is one of the following:
| how | Effect |
| --- | ------------------------ |
| 0 | No future receives |
| 1 | No future sends |
| 2 | No future receives/sends |
The 2 is like `close()`, use `close()`.

### getpeername()

This function is quite simple. It will tell you who is in the other side of the connection.

```c
#include <sys/socket.h>

int getpeername(int sockfd, struct sockaddr *addr, int *addrlen);
```

Example of getting client's IP:

```c
struct sockaddr_storage their;
socklen_t their_len = sizeof their;

int clientfd = accept(sockfd, (struct sockaddr *)&their, &their_len);
getpeername(clientfd, (struct sockaddr *)&their, &their_len);
struct sockaddr_in *client = (struct sockaddr_in *)&their;
char client_ip[INET_ADDRSTRLEN];
inet_ntop(AF_INET, &client->sin_addr, client_ip, INET_ADDRSTRLEN);
```

### gethostname()

```c
#include <unistd.h>

int gethostname(char *hostname, size_t size);
```

Returns the name of the computer that your program is running on.
