********************* CS 525 Advanced Database Organization -- Fall 2017 ****************

==============================================================================================
||								Programming Assignment 1: Storage Manager					||
==============================================================================================



==============================================================================================
* Group member:
	Linlin Chen		A20348195	lchen96@hawk.iit.edu
	Ankush Verma	A20405849	averma15@hawk.iit.edu
	Navneet Goel	A20405197	ngoel2@hawk.iit.edu
	Xiaolin Hu		A20337962	xhu33@hawk.iit.edu

==============================================================================================


==============================================================================================
* How to run the code：
	cd into current folder, then run the following scripts. 
	Compile: make test_assign1
 	Run: ./test_assign1
 	Clean: make clean

 	The code has also alreadly put in 'lchen96' folder in the Fourier server. You can directly test the code there as well.
==============================================================================================



*************** Design Ideas and Implementations ************************
* The goal of assignment 1:
	The goal of this assignment is to implement a simple storage manager - a module that is capable of reading blocks from a file on disk into memory and writing blocks from memory to a file on disk. The storage manager deals with pages (blocks) of fixed size (PAGE_SIZE). In addition to reading and writing pages from a file, it provides methods for creating, opening, and closing files. The storage manager has to maintain several types of information for an open file: The number of total pages in the file, the current page position (for reading and writing), the file name, and a POSIX file descriptor or FILE pointer. 


* Introduction
	In this assignment, we implemented all the interfaces listed in the head file (storage_mgr.h). Our program passed all the test cases. We will use stdlib.h, stdio.h, string.h, math.h header files in storage_mgr.c file. We store the POSIX file descriptor in mgmgInfo in SM_FILEhANDLE structure. 
	Bellow we will introduce our design and implementation ideas for each functions.

* Design idea and implementations:
1. initStorageManager:
	In this function, we do not need any memory initializations. So we only print a welcome sentence for the user. 

2. createPageFile:
	To create a page file, we firstly use fopen function to create a new file with the given name. The file size is PAGE_SIZE bytes. We also fill the new file with '\0' bytes. 

3. openPageFile:
	Two steps are followed here:
	(1). Open the existing file based on the given filename. If the file does not exist, the function will return RC_FILE_NOT_FOUND. Please note here we will implement both read and write operations over this file, so here we need to use fopen with 'r+' to open this file so that we can update it.
	(2). If this file is opened successfully, then we assign the file's information into fHandle parameter. The number of total pages is calculated by dividing the file size with PAGE_SIZE, and we use floor function to get the mathematical floor value. 

4. closePageFile:
	Close the opened file if fHandle is not NULL. If the file is closed successfully, then we assign the fHandle to a meaningless value. 

5. destroyPageFile:
	Simply remove the file with remove function provided by system is enough here.

6. readBlock:
	(1). First we will test whether the required page of the file exists or not, by comparing whether the numPage is negative or exceeds the maximal number of pages. 
	(2). Then we use fseek function to move the file pointer to the corresponding position according to pageNum. 
	(3). After successfully moving the file pointer, we can simply use fread to load the file content into the memory.
	(4). Lastly, modify the current page position into the pageNum. 


7. getBlockPos:
	Simply return the curPagePos in the fHandle. 

8. readFirstBlock:
	The difference between readBlock and readFirstBlock is the value assigned to pageNum. In this function we can simply assign pageNum with value 0 and then call readBlock funciton we have implemented. and return the return value of readBlock.

9. readPreviousBlock:
	Similar to readFirstBlock, we only need to assign the pageNum with curPagePos - 1 and then call the readBlock function.

10. readCurrentBlock:
	Here we assign curPagePos to pageNum and then call the readBlock function.

11. readNextBlock:
	We assign curPagePos + 1 to pageNum and then call the readBlock function.

12. readLastBlock:
	We assign total totalPageNum to pageNum and then call the readBlock function. 

13. writeBlock:
	The design idea here is similar with the designing of readBlock. 
	(1). First we will test whether the required page of the file exists or not, by comparing whether the numPage is negative or exceeds the maximal number of pages. 
	(2). Then we use fseek function to move the file pointer to the corresponding position according to pageNum. 
	(3). After successfully moving the file pointer, we can simply use fwrite to write the content in the page to the file in the disk. 
	(4). Lastly, modify the current page position into the pageNum. 

14. writeCurrentBlock:
	Here we only need to assign the value of pageNum with curPagePos and then call write block function. 

15. appendEmptyBlock:
	(1). We first use fseek to move the file pointer into the end. 
	(2). Then we use fwrite to write a PAGE_SIZE '\0' content into the end of the file. 
	(3). Lastly, we update the totalPageNum by increasing one.

16. ensureCapacity:
	1. Firstly we compare the required numberOfPages with the totalPageNum. If totalPageNum is greater, then nothing is needed to do. 
	2. Otherwise, we simply loop numberOfPages-totalPageNum times and each time call appendEmptyBlock to increase the size of pages to numberOfPages.
















