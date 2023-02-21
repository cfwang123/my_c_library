all: a
a: test.o myjson.o
	gcc --std=gnu99 -O3 -o a test.o myjson.o
test.o: test.c
	gcc --std=gnu99 -O3 -o test.o -c test.c
myjson.o: myjson.c
	gcc --std=gnu99 -O3 -o myjson.o -c myjson.c
