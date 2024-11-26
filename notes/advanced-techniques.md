# Advanced Techniques

- [Advanced Techniques](#advanced-techniques)
    - [Blocking](#blocking)
    - [poll() - Synchronous I/O Multiplexing](#poll---synchronous-io-multiplexing)
    - [epoll() - I/O Event Notification (Async)](#epoll---io-event-notification-async)

### Blocking

All the Unix networking functions are **blocking**. What does it mean? It means that if you write `accept()` or `recv()`
it will wait until some data appears. So how we can avoid this? Making the socket non blocking so we can poll the socket
for info:

```c
fcntl(sockfd, F_SETFL, O_NONBLOCK);
```

`F_SETFL`: set the file descriptor flags  
`O_NONBLOCK`: make the fd non blocking

### poll() - Synchronous I/O Multiplexing

The plan is to have a `struct pollfd` with the info about which sockets fd we want to monitor. The Operating System will
block on the `poll()` call until one event occurs (e.g. "socket ready to write/read").

```c
#include <poll.h>

int poll(struct pollfd fds[], nfds_t nfds, int timeout);
```

`fds` is our array of info (which sockets to monitor)  
`nfds` the elements in the array
`timeout` in milliseconds (-1 to wait forever)

```c
struct pollfd {
    int fd;         // the socket descriptor
    short events;   // bitmap of events we're interested in
    short revents;  // when poll() returns, bitmap of events that occurred
};
```

`events` is a bitmap of the following values:

- `POLLIN` (alert when I can read data)
- `POLLOUT` (alert when I can send data)

**I won't continue this section, read below**

### epoll() - I/O Event Notification (Async)

It is similar to `poll()` but more efficient when dealing with lots of fds. The array is in the kernel space, no further
copies. Nice explaination [here](https://copyconstruct.medium.com/the-method-to-epolls-madness-d9d2d6378642).

```c
#include <sys/epoll.h>

int epoll_create1(int flags);
```

Just pass 0 for the `flags`, it is an improved version of the `epoll_create()`. It creates a new epoll instance and
returns the fd of that instance.

```c
#include <sys/epoll.h>

int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
int epoll_wait(int epfd, struct epoll_event *events, int maxevents, int timeout);
```

- `epoll_ctl()` is used to register new fds and add them to the interest list:
    - `epfd` epoll file descriptor
    - `op` the operation: `EPOLL_CTL_ADD`, `EPOLL_CTL_MOD` or `EPOLL_CTL_DEL`
    - `fd` the file descriptor to check
    - `event` a pointer to the `epoll_event` struct

- `epoll_wait()` waits for I/O events, blocking the calling thread if no events are availables.
    - `epfd` epoll file descriptor
    - `events` array where there will be saved ready events
    - `maxevents` max events per array
    - `timeout` in milliseconds (-1 forever)

```c
struct epoll_event {
    uint32_t events; // bitmap
    epoll_data_t data;
};

union epoll_data {
    void     *ptr;
    int       fd; // just use this
    uint32_t  u32;
    uint64_t  u64;
};
```
