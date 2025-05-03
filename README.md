# cws

A minimal web server. This is a personal project; it is not intended to be a production-ready tool, nor will it ever be. Use it at your own risk.

## Requirements

- [meson](https://mesonbuild.com/index.html)
- libcyaml
- libyaml
- [doxygen](https://www.doxygen.nl/)
    - Optional, just to build the docs.

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
- Support for virtual hosts
- HTTPS support with TLS

## Future
- CLI args
- IPv6 compatible
- Multithreading to handle concurrent requests
- Logging
- Compression (Gzip)
- Reverse proxy
