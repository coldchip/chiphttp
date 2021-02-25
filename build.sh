#!/bin/sh
rep='.o'

for d in ./*.c; do
	echo  gcc -Wall -c -Ofast -s -o bin/$(echo $(basename "$d") | sed "s/\.c/$rep/") $d
    gcc -Wall -c -Ofast -s -o bin/$(echo $(basename "$d") | sed "s/\.c/$rep/") $d
done

echo gcc -o bin/http bin/*.o

gcc -o bin/http bin/*.o -s -Ofast -lpthread