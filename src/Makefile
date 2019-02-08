EXEC=../asbi
VERBOSE=@

# pro .cc ein .o? find-regel?
OBJFILES=tokenizer.o parser.o utils.o ast-optimize.o mem.o vm.o ast.o types.o context.o macros.o procenv.o events/utils.o events/loop.o

ifndef CC
	$(error "do not call this Makefile directly")
endif

ifdef RELEASE
	CPPFLAGS += -O3 -DNDEBUG
	LDFLAGS  += -O3
else
	CPPFLAGS += -Og -g # -fsanitize=address
	LDFLAGS  += -Og -g # -fsanitize=address
	OBJFILES += tests.o
endif

CPPFLAGS += "-DASBI_VERSION=\"$(ASBI_VERSION)\""

.PHONY: all clean

all: $(EXEC)

clean:
	rm -f *.o
	rm -f */*.o
	rm -f *.hh.gch
	rm -f vgcore.* # what are those files?
	rm -f $(EXEC)

$(EXEC): main.o $(OBJFILES)
	@echo " LD  " $@
	$(VERBOSE) $(CPPC) $(LDFLAGS) -o $@ $^

%.o: %.cc
	@echo " C++ " $@
	$(VERBOSE) $(CPPC) $(CPPFLAGS) -c -o $@ $<

events/%.o: events/%.cc
	@echo " C++ " $@
	$(VERBOSE) $(CPPC) $(CPPFLAGS) -pthread -c -o $@ $<

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
