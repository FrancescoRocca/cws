# cws
A simple Web Server written in C (learning purposes), it works only on Linux systems

## Requirements
- [meson](https://mesonbuild.com/index.html)

## How to build
```bash
$ meson setup build
$ cd build
$ meson compile
```
And then run `cws`!

## Roadmap
- [x] Understading basic web server concepts
- [ ] Basic server
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
