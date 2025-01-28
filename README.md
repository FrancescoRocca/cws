# cws

A minimal web server written in C. This is a personal project; it is not intended to be a production-ready tool, nor
will it ever be. Use it at your own risk.

## Requirements

- [meson](https://mesonbuild.com/index.html)
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
# inside the cws directory
$ git submodule update --init
$ doxygen
```

And then open the `docs/html/index.html`.

## Roadmap

- [x] Understanding basic web server concepts
- [x] Basic server
- [ ] CLI args
- [ ] Enhance web server
    - [ ] IPv6 compatible
    - [ ] Request parser
    - [x] Serve static files
    - [ ] Implement Keep-Alive
    - [ ] Multithreading
    - [ ] Logging
- [ ] Advanced
    - [ ] HTTPS support
    - [ ] Caching

## Resources

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)

You can find my journey inside the `notes` directory.
