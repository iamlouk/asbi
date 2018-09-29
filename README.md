# ASBI - A Stack Based Interpreter

```sh
# build:
make

# build with optimizations:
RELEASE=true make --jobs=4

# test:
make test

# run examples:
./asbi --file examples/primes.asbi

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
- `__file`: currently running file
- `__main`: `__file` run from the command line
- `println`: print arguments to `stdout`
- `assert`: throw exception if argument not true
- `mod`: modulo operation for integers

## TODO:
- remove `// TODO:`s in source code
- implement `try`-`catch`?
- new tuple type
- encode stuff like jump location, number of arguments, etc. in opcode
- `++`/increment operator, `<=` and `>=` operators, `...` as tail and by key destruction
- function interface and native c/c++ objects (`Context*`)
- eventloop
- a way to access environment variables and argv, run shell commands, etc.
