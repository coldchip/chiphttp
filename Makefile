.PHONY: clean

all:
	./build.sh compile
run:
	./build.sh run

clean:
	./build.sh clean