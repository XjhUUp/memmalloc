test:TRmalloc.c test-bench.c
	gcc -g -o test.out TRmalloc.c test-bench.c -lpthread

runtests:
	./test.out -a

exec:
	gcc -c -fPIC -o TRmalloc.o TRmalloc.c;
	gcc -shared -o TRmalloc.so TRmalloc.o;

clean:
	rm -f *.o *.out *.so