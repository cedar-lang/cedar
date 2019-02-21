# CEDAR
## A lisp dialect that looks like clojure and feels like go.

Cedar is an interesting lisp interpreter that focuses on concurrency and immutability. It has a coroutine system called "fibers" that cooperatively multitask on thread pools in libUV event loops in order to create highly concurrent programs in lisp
