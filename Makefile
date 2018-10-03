
.PHONY: all clean test asbi-release

SRC=$(shell find src -name "*.cc" -o -name "*.hh")

all: asbi

asbi: $(SRC)
	make -C ./src ../asbi

asbi-release: $(SRC)
	RELEASE=1 make -C ./src --jobs=8 ../asbi

clean:
	make -C ./src clean
	rm -rf vgcore.*

test: asbi
	./asbi --test
