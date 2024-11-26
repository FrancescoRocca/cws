# cws

A simple Web Server written in C (learning purposes), it works only on Linux systems.

## Requirements

- [meson](https://mesonbuild.com/index.html)
- [doxygen](https://www.doxygen.nl/)
    - Optional, just to build the docs

## How to build

```bash
$ meson setup build
$ cd build
$ meson compile
```

And then run `cws`!

## Docs

```bash
$ git submodule update --init # inside the cws directory
$ doxygen
```

And then open the `docs/html/index.html`.

## Roadmap

- [x] Understading basic web server concepts
- [ ] Basic server
- [ ] CLI args
- [ ] Enhance web server
    - [ ] IPv6 compatible
    - [ ] Request parser (methods and headers)
    - [ ] Serve static files
    - [ ] Multithreading (non blocking I/O with `epoll`)
    - [ ] Logging
- [ ] Advanced
    - [ ] HTTPS support
    - [ ] Caching

## Resources

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)

You can find my journey inside the `notes` directory!
