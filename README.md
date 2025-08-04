# cws

A minimal web server. This is a personal project; it is not intended to be a production-ready tool, nor will it ever be. Use it at your own risk.

## Requirements

- [meson](https://mesonbuild.com/index.html)
- libssl
- libcyaml
- libyaml
- [doxygen](https://www.doxygen.nl/)
    - Optional, just to build the docs. It requires `dot`.

## How to build

```bash
$ meson setup build
$ cd build
$ meson compile
```

And then run `cws`!

## Docs

```bash
$ git submodule update --init
$ doxygen
```

And then open the `docs/html/index.html`.

## Roadmap

- Implement Keep-Alive
- HTTPS support with TLS

## Future

- Support for virtual hosts
- Custom web pages (404)
- CLI args
- IPv6 compatible
- Multithreading to handle concurrent requests
- Logging
- Compression (Gzip)
- Reverse proxy

## Performance

This test was performed using `wrk`.

```bash
Running 30s test @ http://localhost:3030
  12 threads and 400 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     3.45ms    2.37ms  91.92ms   95.45%
    Req/Sec    10.08k     1.33k   19.57k    82.30%
  3621422 requests in 32.47s, 2.55GB read
  Socket errors: connect 0, read 0, write 0, timeout 395
Requests/sec: 111514.25
Transfer/sec:     80.51MB
```
