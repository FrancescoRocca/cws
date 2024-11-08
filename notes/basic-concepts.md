# Basic concepts
Before reading, this document could contain errors, please check everything you read.

- [Basic concepts](#basic-concepts)
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


### Socket
When Unix programs do some I/O they do it reading/writing to a **file descriptor**. A file descriptor is an integer associated with an open file, it can be anything. To communicate over the internet using a file descriptor we'll make a call to the `socket()` system routine.

### Internet sockets
There are two types of Internet sockets:
1. Stream Sockets (SOCK_STREAM) - error-free and a realiable two-way communication (TCP)
2. Datagram Sockets (SOCK_DGRAM) - connectionless (UDP)

### Byte Order
- Big-Endian (also called **Network Byte Order**)
- Little-Endian

Before making any transmission we have to convert the byte order to a Network Byte Order, we can do this with simple functions:

| Function | Description           |
| -------- | --------------------- |
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
But, we can avoid to pack manually the stuff inside this struct and use the `struct sockaddr_in` (or `struct sockaddr_in6` for IPv6) with a fast cast that is made for the Internet:

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
This struct is designed to storage both IPv4 and IPv6 structures. Example, when a client is going to connect to your server you don't know if it is a IPv4 or IPv6 so you use this struct and then cast to what you need (check the `ss_family` first).

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

There is a very easy function called `inet_pton()`, *pton* stands for **Presentation to network**. If you want to do the same but from binary to string you have `inet_ntop()`.

### getaddrinfo()
```c
int getaddrinfo(const char *node,     // e.g. "www.example.com" or IP
                const char *service,  // e.g. "http" or port number
                const struct addrinfo *hints,
                struct addrinfo **res);
```

The `node` could be a host name or IP address. `service` could be a port number or a service found in `/etc/services` (e.g. "http" or "ftp" or "telnet").
`hints` points to a struct you already filled and `res` contains a linked list of results.