# cws

A minimal web server. This is a personal project; it is not intended to be a production-ready tool, nor will it ever be. Use it at your own risk.

## Requirements

- [meson](https://mesonbuild.com/index.html)
- libcyaml
- myclib (on my profile)
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

### Doing

- Support for virtual hosts

## Todo

- Minimal Templating
- IPv6 compatible

## Performance

This test was performed using [goku](https://github.com/jcaromiq/goku).

<details>
    <summary>goku command</summary>

```bash
$ goku -t http://localhost:3030 -c 400 -d 30
```

</details>

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
