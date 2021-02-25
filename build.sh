#!/bin/sh

OUTPUT="http"

compile() {
	rep='.o'

	for d in ./*.c; do
		echo  gcc -Wall -c -Ofast -s -o bin/$(echo $(basename "$d") | sed "s/\.c/$rep/") $d
	    gcc -Wall -c -Ofast -s -o bin/$(echo $(basename "$d") | sed "s/\.c/$rep/") $d
	done

	echo gcc -o bin/$OUTPUT bin/*.o

	gcc -o bin/$OUTPUT bin/*.o -s -Ofast -lpthread
}

run() {
	./bin/$OUTPUT
}

clean() {
	rm -rf bin/*
}

"$@"