# ASBI - A Stack Based Interpreter

```sh
# build:
make

# build with optimizations:
RELEASE=true make --jobs=4

# test:
make test

# run examples:
./asbi --file examples/examples.asbi

# development:
(cd ./src && make) && valgrind ./asbi --test --file examples/test.asbi
```

```js
// example:

fibs := [1, 1];
fib := (i) -> if fibs.i != nil { fibs.i } else { fibs.i = fib(i - 1) + fib(i - 2) };

assert("fibonacci sequence", fib(3) + fib(4) = fib(5));
```

## Globals/Constants/Builtins:
- look at the outer most `:outer_scope` in `__scope()`

## TODO:
- remove `// TODO:`s in source code
- implement `try`-`catch`?
- new tuple type
- encode stuff like jump location, number of arguments, etc. in opcode
- `++`/increment operator, `...` as tail and by key destruction
- `await`: replace `readline((line) -> {...})` with `line := await readline()`?
- function interface and native c/c++ objects (`Context*`)
- move constructor for `StringContainer`
- `::`/bind operator: `foo := (this, arg) -> this; obj::foo(arg) == obj`
	- `"hallo welt"::str:split(" ")`
- system()-function
- eventloop
	- intervals
- plugin interface
