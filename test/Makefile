all:function_test seu_test log_test

function_test:../src/memmalloc.c function_test.c
	gcc -g -o function.out ../src/memmalloc.c function_test.c  -I../include -lpthread

seu_test:../src/memmalloc.c seu_test.c
	gcc -g -o seu_test.out ../src/memmalloc.c seu_test.c -DSEU -I../include -lpthread

tmr_test:../src/memmalloc.c tmr_test.c
	gcc -g -o tmr_test.out ../src/memmalloc.c tmr_test.c -DSEU -I../include -lpthread

log_test:../src/memmalloc.c log_test.c
	gcc -g -o log.out ../src/memmalloc.c ../src/log.c log_test.c  -DLOG -DFILE_NAME='"log.out"' -I../include -lpthread

run_function_test:function_test
	./function.out

run_seu_test: seu_test
	./seu_test.out

run_tmr_test:tmr_test
	./tmr_test.out

run_log_test:log_test
	./log.out






clean:
	rm -f *.o *.out *.so