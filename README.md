# cws

A minimal HTTP web server written in C.

> **Note**: This is a personal project; it is not intended to be a production-ready tool, nor will it ever be. Use it at your own risk.

## Requirements

- myclib (on my profile)
- [tomlc17](https://github.com/cktan/tomlc17)

## Build

```bash
meson setup build
meson compile -C build
```

## Usage

1. Run `./build/cws`
2. Open `http://localhost:3030` in your browser

## Performance

Tested with [goku](https://github.com/jcaromiq/goku) (`goku -c 400 -d 30 -t http://localhost:3030`):

```bash
Concurrency level 400
Time taken       30 seconds
Total requests   388439
Requests/sec     12926.60 req/s
Mean             30.38 ms
Min              0 ms
Max              341 ms
p50 (median)     26 ms
p95              73 ms
p99              100 ms
p99.9            128 ms

Status codes
  2xx 388439
```
