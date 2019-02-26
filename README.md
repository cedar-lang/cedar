# CEDAR
## A lisp dialect that looks like clojure and feels like go.

Cedar is an interesting lisp interpreter that focuses on concurrency and immutability. It has a coroutine system called "fibers" that cooperatively multitask on thread pools in libUV event loops in order to create highly concurrent programs in lisp

### Building
```sh
# to clone with dependencies
$ git clone --recursive git@github.com:cedar-lang/cedar.git
$ cd cedar
# compile
$ make
# install
$ sudo make install
```


---
[![FOSSA Status](https://app.fossa.io/api/projects/git%2Bgithub.com%2Fnickwanninger%2Fcedar.svg?type=large)](https://app.fossa.io/projects/git%2Bgithub.com%2Fnickwanninger%2Fcedar?ref=badge_large)
