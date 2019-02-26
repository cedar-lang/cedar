# CEDAR
## A lisp dialect that encourages async via concurrency

Cedar is an interesting lisp interpreter that focuses on concurrency and
immutability. It has a coroutine system called "fibers" that cooperatively
multitask in libuv event loops in order to create highly
concurrent programs in lisp

```clojure

;; TODO: put an example here :)

```



### Building

The cedar build process uses CMake, but I never really liked how cmake requires
the user to create a `build` directory so the `Makefile` does all that for you.
```sh
# to clone with dependencies
$ git clone --recursive git@github.com:cedar-lang/cedar.git
$ cd cedar
# compile
$ make
# install
$ sudo make install
```



### Syntax Highlighting
Cedar's syntax will highlight correctly with practically any lisp, but because
it's grammar and syntax best resembles Clojure's `.edn` format, its usally best
to use that here. In vim, I simply set it's filetype to clojure:

``vim
au BufRead,BufNewFile *.cdr set filetype=clojure
```

This might cause errors if you have special Clojure tools in vim, but it's
worked for me so far


---
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2Fnickwanninger%2Fcedar.svg?type=large)](https://app.fossa.io/projects/git%2Bgithub.com%2Fnickwanninger%2Fcedar?ref=badge_large)
