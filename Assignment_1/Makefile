CC=gcc
CFLAGS=-lm


test_assign1: test_assign1_1.o storage_mgr.o dberror.o
	$(CC) -o $@ $^ $(CFLAGS)

dberror.o: dberror.c dberror.h
	$(CC) -c $< 

storage_mgr.o: storage_mgr.c storage_mgr.h dberror.h
	$(CC) -c $< 

test_assign1_1.o: test_assign1_1.c test_helper.h
	$(CC) -c $<

clean:
	rm *.o test_assign1




