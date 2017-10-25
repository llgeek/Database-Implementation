CC=gcc
CFLAGS=-lm


test_assign2: test_assign2_1.o buffer_mgr.o buffer_mgr_stat.o dberror.o
	$(CC) -o $@ $^ $(CFLAGS)

dberror.o: dberror.c dberror.h
	$(CC) -c $< 

buffer_mgr_stat.o: buffer_mgr_stat.c buffer_mgr_stat.h buffer_mgr.h
	$(CC) -c $<

buffer_mgr.o: buffer_mgr.c storage_mgr.h dberror.h dt.h
	$(CC) -c $< 

test_assign2_1.o: test_assign2_1.c test_helper.h storage_mgr.h buffer_mgr_stat.h buffer_mgr.h dberror.h
	$(CC) -c $<

clean:
	rm *.o test_assign2




