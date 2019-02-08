# ASBI - A Stack Based Interpreter

```sh
# build:
make

# build with optimizations:
RELEASE=true make --jobs=4

# run examples:
./asbi examples/examples.asbi
```

## Example
```js
// comment

fibs := [1, 1];
fib := (i) -> if fibs.i != nil { fibs.i } else { fibs.i = fib(i - 1) + fib(i - 2) };

assert("fibonacci sequence", fib(3) + fib(4) = fib(5));
```

## Globals/Constants/Builtins:
- look at the outer most `:outer_scope` in `__scope()`

## TODO:
- remove `// TODO:`s in source code
- implement `try`/`catch`?
- new tuple type
- encode stuff like jump location, number of arguments, etc. in opcode
- `++`/increment operator, `...` as tail and by key destruction
- `await`: instated of `readline((line) -> {...})` do `line := await readline()`?
- function interface and native c/c++ objects (`Context*`)
- move constructor for `StringContainer`
- `::`/bind operator: `foo := (this, arg) -> this; obj::foo(arg) == obj`
	- `"hallo welt"::str:split(" ")`
- plugin interface
- error handling:
	- `src/utils.cc`
	- `readline`
- Makefile: build directory
