#include <iostream>
#include <cassert>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include "include/types.hh"
#include "include/vm.hh"
#include "include/utils.hh"

#ifdef NDEBUG
#error
#endif

using namespace asbi;

namespace tests {

	void test(const std::string code, Value expected) {
		Context ctx;
		std::cout << "test(\'" << code << "\'): " << std::flush;
		Value res = ctx.run(code);
		assert(res == expected);
		std::cout << "SUCCESS\n";
	}

	void run(){

		test("(true & true) & !((false & true) | (true  & false) | (false & false))", Value::boolean(true));
		test("(true | true) & (false | true) & (true | false) & !(false | false)", Value::boolean(true));
		test("3 - 2 == 1 & 6 / 2 == 3 & 1 + 2 == 3 & 3.14 > 0.1 + 2 & 10 < 100", Value::boolean(true));
		test("(if true { 1 } else { 2 }) == 1 & (if false { 2 } else {}) == nil", Value::boolean(true));
		test("if false { 1 } else if true { true & false; 3.14 + 42; 2 } else { 3 }", Value::number(2));
		test(":hello_world != \"Hallo Welt!\" & (\"hello\" + \"world\") == \"helloworld\"", Value::boolean(true));
		test("a := 6; a = 42; a = a == 42 & { b := 12; b } == 12; a & { a == true }", Value::boolean(true));
		test("sum := 0; for i := 1; i < 6; i := i + 1 { sum = sum + i; }; sum", Value::number(15));
		test("add := (a, b) -> a + b, sub := (a, b) -> a - b; add(28, 14) == sub(45, 3)", Value::boolean(true));
		test("fib := (n) -> if n < 2 { 1 } else { fib(n - 1) + fib(n - 2) }; fib(5) == fib(4) + fib(3)", Value::boolean(true));
		test("fibGen := ()->{a := 1, b := 1; ()->{tmp := a; a = a + b; b = tmp}}; fibs := fibGen(); fibs()+fibs()==fibs()", Value::boolean(true));
		test("arr := [\"hello\", :symbol, 3.14, 42], map := [1234 ~ \"value\", 42 ~ [,], :key ~ 42]; (arr.3 + map.:key) / 2", Value::number(42));
		test("arr := [:key ~ \"value\"]; arr.:key == \"value\" & { arr.:key = 42; arr.:key == 42 }", Value::boolean(true));
		test("(\"hallo\" + (\" \" + \"welt\")) == \"hallo welt\" & :test == { a := :test; a }", Value::boolean(true));
		test("fibs := [1, 1], fib := (n) -> if fibs.n != nil { fibs.n } else { fibs.n = fib(n - 1) + fib(n - 2) }; fib(5)", Value::number(8));
		test("if nil == {} {} else { [:would_fail] := [:no_match] }", Value::nil());
		test("[a, :test, b, 123] := ((x) -> [x, :test, :b, 123])(42); a == 42 & b == :b", Value::boolean(true));
		test("[1, 4, 9, 16, 25] := map([1, 2, 3, 4, 5], (_, x) -> x * x); nil", Value::nil());
		test("fold([1, 2, 3, 4, 5], 0, (sum, _, x) -> sum + x)", Value::number(15));

	}

}
