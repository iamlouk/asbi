CPPC     = clang
CPPFLAGS = -std=c++17 -Wall -Wextra -D_XOPEN_SOURCE=700
LDFLAGS  = -pthread -lstdc++ -lm -lreadline
ASBI_VERSION = 0.2.1

ifdef GPROF
CPPFLAGS += -pg
LDFLAGS += -pg
# @echo "Profile: gprof asbi gmon.out | grep -v "std::\|cxx"  | less"
endif

export CPPC
export CPPFLAGS
export LDFLAGS
export ASBI_VERSION

.PHONY: all clean test asbi-release

SRC=$(shell find src -name "*.cc" -o -name "*.hh")

all: asbi

asbi: $(SRC)
	$(MAKE) --jobs=4 -C ./src ../asbi

asbi-release: $(SRC)
	RELEASE=defined make -C ./src --jobs=8 ../asbi

clean:
	$(MAKE) -C ./src clean
	rm -rf vgcore.*

test: asbi
	./asbi --test
