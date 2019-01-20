# Cedar Lisp
### A lisp implementation in c++ that kinda looks like clojure
---

Here's a quick example of the syntax:
```clojure
; normal lisp function application
(+ 1 2)
; normal looking funciton literals
(fn (x) x)
; variable setting
(def foo "bar")
; function setting macro
(defn id (x) x)

;; classes
; define
(class person (name)
  ((say (fn (thing) (print name "says '" thing "'")))))
; instantiate
(def bob (person "Bob"))
; call
(bob.say "Hello World!)
; prints: "Bob says 'Hello World!'"
```

It's still in its early days, so I'll update this README as features get implemented
