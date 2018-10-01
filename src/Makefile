CPPC=clang
CPPFLAGS= -std=c++17 -Wall -Wextra
LDFLAGS=  -std=c++17 -pthread -lstdc++ -lm

EXEC=../asbi
OBJFILES=tokenizer.o parser.o utils.o ast-optimize.o mem.o vm.o ast.o types.o context.o macros.o procenv.o

ifdef RELEASE
	CPPFLAGS += -O3 -DNDEBUG
	LDFLAGS  += -O3
else
	CPPFLAGS += -O0 -g # -fsanitize=address # (does not work with valgrind)
	LDFLAGS  += -O0 -g # -fsanitize=address
	OBJFILES += tests.o
endif

.PHONY: all clean

all: $(EXEC)

clean:
	rm -f *.o
	rm -f *.hh.gch
	rm -f vgcore.* # what are those files?
	rm -f $(EXEC)

$(EXEC): main.o $(OBJFILES)
	$(CPPC) $(LDFLAGS) -o $@ $^

%.o: %.cc
	$(CPPC) $(CPPFLAGS) -c -o $@ $<

tokenizer.o: tokenizer.cc tokenizer.hh utils.hh
parser.o: parser.cc parser.hh ast.hh tokenizer.hh utils.hh context.hh
utils.o: utils.cc utils.hh ast.hh tokenizer.hh
ast-optimize.o: ast-optimize.cc ast.hh
mem.o: mem.cc mem.hh context.hh
vm.o: vm.cc vm.hh types.hh context.hh
ast.o: ast.cc ast.hh vm.hh context.hh
types.o: types.cc types.hh vm.hh mem.hh context.hh
context.o: context.cc context.hh types.hh vm.hh
main.o: main.cc context.hh types.hh utils.hh procenv.hh
tests.o: tests.cc context.hh types.hh vm.hh utils.hh parser.hh tokenizer.hh
macros.o: macros.cc context.hh utils.hh types.hh
procenv.o: procenv.cc procenv.hh context.hh types.hh

