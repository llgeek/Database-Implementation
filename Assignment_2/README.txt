* Design idea and implementations:
1. initBufferPool():
  In this function, we initialized a buffer pool. We used an array to store the BM frames. 
  Each frame contians its page data, fixing and dirty information, loading time and lastly used time for the replacement strategy. 
  
2. shutdownBufferPool():
  In this function, we freeed the memory used by the given buffer pool.
  A buffer pool should be unpinned and all dirty pages were written back to file before shut down. An error will be present if we tried to shut down a buffer that is used by someone.
3. forceFlushPool():
  We checked all the frames in the buffer pool. if a frame has an dirty page and is unpinned, write it back to the file.
