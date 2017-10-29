CC=gcc
CFLAGS=-lm
STD=-std=gnu99


test_assign2: test_assign2_1.o buffer_mgr.o buffer_mgr_stat.o dberror.o storage_mgr.o
	$(CC) -o $@ $^ $(CFLAGS) $(STD)

dberror.o: dberror.c dberror.h
	$(CC) -c $< $(STD)

storage_mgr.o: storage_mgr.c storage_mgr.h
	$(CC) -c $< $(STD)

buffer_mgr_stat.o: buffer_mgr_stat.c buffer_mgr_stat.h buffer_mgr.h
	$(CC) -c $< $(STD)

buffer_mgr.o: buffer_mgr.c storage_mgr.h dberror.h dt.h
	$(CC) -c $< $(STD)

test_assign2_1.o: test_assign2_1.c test_helper.h storage_mgr.h buffer_mgr_stat.h buffer_mgr.h dberror.h
	$(CC) -c $< $(STD)

clean:
	rm *.o test_assign2 




