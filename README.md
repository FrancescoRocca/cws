# cws

A minimal HTTP web server written in C.

> **Note**: This is a personal project; it is not intended to be a production-ready tool, nor will it ever be. Use it at your own risk.

## Requirements

- myclib (on my profile)
- [tomlc17](https://github.com/cktan/tomlc17)
- [doxygen](https://www.doxygen.nl/) (optional, for documentation only - requires `dot`)

## Build

```bash
meson setup build
meson compile -C build
```

## Usage

1. Copy `config.yaml` and `www/` directory to your working directory
2. Run `./build/cws`
3. Open `http://localhost:3030` in your browser

## Documentation

```bash
git submodule update --init
doxygen
```

Then open `docs/html/index.html`.

## Roadmap

- [ ] Virtual hosts support
- [ ] Minimal templating engine
- [ ] IPv6 compatibility

## Performance

Tested with [goku](https://github.com/jcaromiq/goku) (`-c 400 -d 30`):

```bash
Concurrency level 400
Time taken 31 seconds
Total requests  365363
Mean request time 22.665250723253322 ms
Max request time 2067 ms
Min request time 0 ms
95'th percentile: 20 ms
99.9'th percentile: 1078 ms
200 OK 365363
```
