CC=gcc
CFLAGS=-lm
STD=-std=gnu99

all: test_assign3 test_expr

test_assign3: test_assign3_1.o record_mgr.o expr.o rm_serializer.o buffer_mgr.o buffer_mgr_stat.o dberror.o storage_mgr.o
	$(CC) -o $@ $^ $(CFLAGS) $(STD)

test_expr: test_expr.o dberror.o expr.o record_mgr.o rm_serializer.o
	$(CC) -o $@ $^ $(CFLAGS) $(STD)

expr.o: expr.c expr.h dberror.h record_mgr.h tables.h
	$(CC) -c $< $(STD)

rm_serializer.o: rm_serializer.c dberror.h tables.h record_mgr.h
	$(CC) -c $< $(STD)

dberror.o: dberror.c dberror.h
	$(CC) -c $< $(STD)

storage_mgr.o: storage_mgr.c storage_mgr.h
	$(CC) -c $< $(CFLAGS) $(STD) 

buffer_mgr_stat.o: buffer_mgr_stat.c buffer_mgr_stat.h buffer_mgr.h
	$(CC) -c $< $(STD)

buffer_mgr.o: buffer_mgr.c buffer_mgr.h storage_mgr.h
	$(CC) -c $< $(STD)

record_mgr.o: record_mgr.c record_mgr.h dberror.h expr.h tables.h storage_mgr.h buffer_mgr.h buffer_mgr_stat.h
	$(CC) -c $< $(STD)

test_expr.o: test_expr.c dberror.h expr.h record_mgr.h tables.h test_helper.h storage_mgr.h buffer_mgr_stat.h buffer_mgr.h dt.h
	$(CC) -c $< $(STD)

test_assign3_1.o: test_assign3_1.c dberror.h expr.h record_mgr.h tables.h test_helper.h storage_mgr.h buffer_mgr_stat.h buffer_mgr.h dt.h
	$(CC) -c $< $(STD)

clean:
	rm *.o test_assign3




