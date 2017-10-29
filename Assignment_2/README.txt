********************* CS 525 Advanced Database Organization -- Fall 2017 ****************

==============================================================================================
||								Programming Assignment 2: Buffer Manager					||
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
	Compile: make test_assign2 (or simply input: make)
 	Run: ./test_assign2
 	Clean: make clean

 	The code has also alreadly been put in 'lchen96' folder in the Fourier server. You can directly test the code there as well.
==============================================================================================



*************** Design Ideas and Implementations ************************
* The goal of assignment 2:
	The goal of this assignment is to implement a simple buffer manager - a module that is capable of managing a fixed number of pages in memory that represent pages from a page file managed by the storage manager implemented in assignment 1. The memory pages managed by the buffer manager are called page frames or frames for short. We call the combination of a page file and the page frames storing pages from that file a Buffer Pool. The Buffer manager should be able to handle more than one open buffer pool at the same time. However, there can only be one buffer pool for each page file. Each buffer pool uses one page replacement strategy that is determined when the buffer pool is initialized.


* Introduction
	In this assignment, we implemented all the interfaces listed in the head file (buffer_mgr.h). Our program passed all the test cases. We use stdlib.h, stdio.h, string.h header files in buffer_mgr.c file. 
	Bellow we will introduce our design and implementation ideas for each function.


* Design idea and implementations:
To store some bookkeeping data, we define two new struct type: BM_FrameHandle and BM_MgmtData.

typedef struct BM_FrameHandle{
    BM_PageHandle *pgdata;
    int fix_count; 
    bool is_dirty; 
    int load_time;
    int used_time;
} BM_FrameHandle;
This struct is used to store the contens of each frame in the bufer. 
pgdata is used to store the BM_PageHandle data, which stores the pageNum and data content;
fix_count is used to store the number of clients using this buffer; 
is_dirty is used to represent whether this frame is modified by some client; 
load_time is used to store the time when the page in this frame is loaded, this will be used for FIFO; 
used_time is used to store the time when the page in this frame is used, this will be used for LRU;


typedef struct BM_MgmtData {
    BM_FrameHandle *frames; 
    SM_FileHandle *fileHandle;
    int read_times;
    int write_times;
    int *frame2page;
} BM_MgmtData;
This struct is used to store the bookkeeping data, for mgmtData pointer in BM_BufferPool.
frames is used to store the frame contents, this should be an array, length equals the numPages;
fileHandle is used to store the file handle related to the file in the buffer. 
read_times is used to store the number of IO reads from disk to memory of this buffer;
write_times is used to store the number of IO writes from meomry to disk of this buffer;
frame2page is used as a mapping from framenum to pagenum; This is for fast lookup.



1. initBufferPool():
  In this function, we initialized a buffer pool. We used an array to store the BM frames. 
  Each frame contians its page data, fixing and dirty information, loading time and lastly used time for the replacement strategy. We will malloc all the space required for each pointer and initialize them.
  
2. shutdownBufferPool():
  In this function, we freeed the memory used by the given buffer pool.
  A buffer pool should be unpinned and all dirty pages were written back to file before shut down. An error will be present if we tried to shut down a buffer that is used by someone. We will free all the space we malloced in initBufferPool function and destroy the page file we created.


3. forceFlushPool():
  We checked all the frames in the buffer pool. If a frame has a dirty page and is unpinned, write it back to the file and increment the number of write_times; We will iterate this along all buffer frames and write all dirty frames to disk.


4. pinPage():
	To pin a page, firstly we need to check whether this page already exists in the buffer. Since we keep a mapping from framenum to pagenum, iterating the frame2page will help us determine whether it's in the buffer. If so, increment the fix_count, assign the value to page, and directly return. If not, we have to find a frame to load this page. I write two seperate functions, replacewithFIFO and replacewithLRU, to help us find the corresponding frame to load the frame. replacewithFIFO or replacewithLRU will return a framenum. If the framenum is not -1, then we will load the page to that frame and assign all corresponding values stored in that frame.  If it's -1, this means we cannot find a frame to load the page. Finally we need to assign the pagenum and content to the page. 

5. replacewithFIFO():
	This function is used for FIFO replacement strategy. Firstly we need to search whether there exist one frame with no page in. If we can find such frame, directly return its framenum and we do not need to replace any frame. If not, we have to use FIFO strategy to find the first loaded frame to be replaced. Since we keep load_times for each frame to help us record each frame's loading time, we can directly iterate it to find the frame with smallest load_time. That would be the one to be replaced. Note if the frame to be replaced is dirty, we need to firstly write current content back to the disk, and then replace it with the desired page.

6. replacewithLRU():	
	This function is used for LRU replacement strategy. Firstly we need to search whether there exist one frame with no page in. If we can find such frame, directly return its framenum and we do not need to replace any frame. If not, we have to use LRU strategy to find the leastly used frame to be replaced. Since we keep used_times for each frame to help us record each frame's latest used time, we can directly iterate it to find the frame with smallest used_time. That would be the one to be replaced. Note if the frame to be replaced is dirty, we need to firstly write current content back to the disk, and then replace it with the desired page.

7. unpinPage():
	To unpin a page, we need firstly find the frame caching the page. If we cannot find a frame caching the page, we will return an error code implying not being able to find such frame. If we sucessfully find the frame, decrementing its fix_count by one is enough.

8. markDirty():
	Similar with unpinPage, we need to firstly find the frame caching the page. If we sucesfully find the frame, mark its is_dirty with true and write the page content to this frame. This is needed because we will rely on this frame to write the dirty frame's content back to the disk.


9. forcePage():
	Firstly we need to get the framenum caching this page based on the pagenum. If we sucessfuly find the frame, then we will firstly ensure the pagefile's size is not smaller than the pagenum. If so, we use ensureCapacity to increase the pagefile's size. Then we call the writeBlock to write the frame's contents back to the disk file. We need to increment the write_times by one here.


10. getFrameContents():
	In the buffer handle, we keep a mapping from frame num to page num, named frame2page. Directly return frame2page is enough.


11. getDirtyFlags():
	Iterate the buffer and assign each frame's is_dirty value to the return array. Since we already keep the is_dirty up-to-date for each frame, one iteration is enough.

12. getFixCounts():
	Iterate the buffer and assign each frame's fix_count value to the return array. Since we already keep the fix_count up-to-date for each frame, one iteration is enough.

13. getNumReadIO():
	Directly return the read_times stored in the frame is enough.

14. getNumWriteIO():
	Directly return the write_times stored in the frame is enough.




