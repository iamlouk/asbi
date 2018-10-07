CPPC=clang
CPPFLAGS= -std=c++17 -Wall -Wextra
LDFLAGS=  -std=c++17 -pthread -lstdc++ -lm -lreadline

EXEC=../asbi
OBJFILES=tokenizer.o parser.o utils.o ast-optimize.o mem.o vm.o ast.o types.o context.o macros.o procenv.o events/utils.o events/loop.o

ifdef RELEASE
	CPPFLAGS += -O3 -DNDEBUG
	LDFLAGS  += -O3
else
	CPPFLAGS += -O0 -g# -fsanitize=address
	LDFLAGS  += -O0 -g# -fsanitize=address
	OBJFILES += tests.o
endif

.PHONY: all clean

all: $(EXEC)

clean:
	rm -f *.o
	rm -f */*.o
	rm -f *.hh.gch
	rm -f vgcore.* # what are those files?
	rm -f $(EXEC)

$(EXEC): main.o $(OBJFILES)
	$(CPPC) $(LDFLAGS) -o $@ $^

%.o: %.cc
	$(CPPC) $(CPPFLAGS) -c -o $@ $<

events/%.o: events/%.cc
	$(CPPC) $(CPPFLAGS) -pthread -c -o $@ $<

tokenizer.o: tokenizer.cc include/tokenizer.hh include/utils.hh
parser.o: parser.cc include/parser.hh include/ast.hh include/tokenizer.hh include/utils.hh include/context.hh
utils.o: utils.cc include/utils.hh include/ast.hh include/tokenizer.hh
ast-optimize.o: ast-optimize.cc include/ast.hh
mem.o: mem.cc include/mem.hh include/context.hh
vm.o: vm.cc include/vm.hh include/types.hh include/context.hh
ast.o: ast.cc include/ast.hh include/vm.hh include/context.hh
types.o: types.cc include/types.hh include/vm.hh include/mem.hh include/context.hh
context.o: context.cc include/context.hh include/types.hh include/vm.hh
main.o: main.cc include/context.hh include/types.hh include/utils.hh include/procenv.hh
tests.o: tests.cc include/context.hh include/types.hh

macros.o: macros.cc include/context.hh include/utils.hh include/types.hh events/utils.hh events/loop.hh
procenv.o: procenv.cc include/procenv.hh include/context.hh include/types.hh

events/utils.o: events/utils.cc events/utils.hh
events/loop.o: events/loop.cc events/loop.hh events/utils.hh include/context.hh include/types.hh
