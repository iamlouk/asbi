
.PHONY: all clean test asbi-release

all: asbi

asbi:
	make -C ./src ../asbi

asbi-release:
	RELEASE=1 make -C ./src --jobs=4 ../asbi

clean:
	make -C ./src clean
	rm -rf vgcore.*

test: asbi
	valgrind ./asbi --test
