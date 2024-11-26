# HTTP Request

This is an example of a basic HTTP request made from the browser:

```bash
GET / HTTP/1.1
User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)
Host: www.tutorialspoint.com
Accept-Language: en-us
Accept-Encoding: gzip, deflate
Connection: Keep-Alive
```

> Thanks tutorialspoint

The first line is a *request line*. It has:

- Method (GET, POST, HEAD, ...)
- Location (the request resource, file)
- HTTP version

# HTTP Response

```bash
HTTP/1.1 200 OK\r\n
Content-Type: text/html\r\n
Content-Length: 88\r\n
Connection: Closed\r\n
\r\n
<HTML>
```

`Content-Length` is the length of the body (in the response case is the length of html page)