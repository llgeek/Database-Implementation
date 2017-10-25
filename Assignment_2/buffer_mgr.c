#include <stdio.h>
#include <strlib.h>

#include "buffer_mgr.h"
#include "storage_mgr.h"


/********************************************************
 Define the struct BM_FrameHandle
 Xiaolin hu
 ********************************************************/
typedef struct BM_FrameHandle{
    PageNumber pageNum;
    char *data;
    
    int fix_count; //starting from 0
    bool is_dirty; //true is dirty, false is clean
    
} BM_FrameHandle

/********************************************************
Define the struct mgmtData
Xiaolin hu
********************************************************/
typedef struct BM_MgmtData { //The bookkeeping info
    
    //using a BM_FrameHandle and a SM_FileHandle to store the pages and current file info
    BM_FrameHandle *frames; //Actually it's an array of frames
    SM_FileHandle *fileHandle;
    
    int read_times;  //getNumReadIO()
    int write_times;  //getNumWriteIO()
	

} BM_mgmtData;

/********************************************************
 Define an error type
 Xiaolin hu
 ********************************************************/
#define RC_BM_PINNED 5

// Buffer Manager Interface Pool Handling
/********************************************************
initBufferPool()
Xiaolin Hu
*********************************************************/
RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName, 
		  const int numPages, ReplacementStrategy strategy, 
		  void *stratData){


   //check if the file is OK to open
   //Using the storge manager
	SM_FileHandle *fHandle = (SM_FileHandle *)malloc(SM_FileHandle);

	RC ret = openPageFile(pageFileName, fHandle);
	if (ret != RC_OK) {
		return ret;
	}

    //Initial an array to store the BM frames
    BM_FrameHandle *BM_frames = (BM_FrameHandle *)malloc(sizeof(BM_FrameHandle) * numPages);
    for (int i=0; i<numPages; i++){
        
        //repeatedly create new memory space to filepage and assign it to the frames
        SM_PageHandle fpage = (SM_PageHandle)malloc(PAGE_SIZE);
        
        BM_frames[i].pageNum = NO_PAGE;   //at beginning, set all page number to be -1.
        BM_frames[i].data = fpage;
        BM_frames[i].fix_count = 0;
        BM_frames[i].is_dirty = 0;
    
    }
    
    //Using mgmtData to store the bookkeeping info
    BM_MgmtData *mgmtData = (BM_MgmtData *)malloc(sizeof(BM_MgmtData));
    mgmtData -> fileHandle = fHandle;
    mgmtData -> frames = BM_frames;
    mgmtData -> read_times = 0;
    mgmtData -> write_times =0;
    
    bm -> mgmtData = mgmtData;

	//Initial the others
	bm -> pagefile = pageFileName;
	bm -> numPages = numPages;
	bm -> strategy = strategy;

	return RC_OK;

}

/********************************************************
 shutdownBufferPool()
 Xiaolin Hu
 *********************************************************/
RC shutdownBufferPool(BM_BufferPool *const bm){
    
    BM_MgmtData mgmtData = (BM_MgmtData*)bm -> mgmtData; //need casting
    BM_FrameHandle frames = mgmtData -> frames;
    
    SM_FileHandle file = mgmtData -> fileHandle;
    int numPages = bm -> numPages;
    
    //Check if there's any pinned pages
    for(int i=0 ; i<numPages ; i++){
        if(frames[i].fix_count != 0)
            return RC_BM_PINNED；
    }
    //Force flush pool before destroy
    forceFlushPool(bm);
    
    //Free the malloc spaces in the Pool
    free(frames);
    free(fHandle);
    free(mgmtData);
    
    return RC_OK;
    
}
/********************************************************
 forceFlushPool()
 Xiaolin Hu
 *********************************************************/
RC forceFlushPool(BM_BufferPool *const bm){
    
    BM_MgmtData mgmtData = (BM_MgmtData*)bm -> mgmtData; //need casting
    BM_FrameHandle frames = mgmtData -> frames;
    
    SM_FileHandle file = mgmtData -> fileHandle;
    int numPages = bm -> numPages;
    
    for(int i=0 ; i<numPages ; i++){
        if(frames[i].fix_count == 0 && frames[i].is_dirty == true){
            int pageN = frames[i].pageNum;
            SM_PageHandle page = frames[i].data;
            writeBlock (pageN, file, page);  //write back the page to file
            frames[i].is_dirty=false;  //reset the dirty flag
            
            mgmtData -> write_times++; // accumulate the num of write times
        }
    }
    
    return RC_OK;
    
}




//Start Buffer Manager Interface Access Pages
/**
 * Linlin Chen
 * A20348195
 * lchen96@hawk.iit.edu
 */

/**
 * pins the page with page number pageNum
 * @param  bm: the buffer manager pool
 * @param  page: the page stored the page file content
 * @return      any error code
 */
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, 
	    const PageNumber pageNum) {


}


/**
 * unpins the page page
 * @param  bm: the buffer manager pool
 * @param  page: the page handle, the pagenum will be used to figure out which page will be unpined
 * @return      error code
 */
RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page) {

}


/**
 * marks a page as dirty.
 * @param  bm: buffer manager
 * @param  page: page handle
 * @return    error code
 */
RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page) {

}


/**
 * write the current content of the page back to the page file on disk
 * @param  bm: the buffer manager
 * @param  page: the page handle
 * @return      error code
 */
RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page) {

}
//End Buffer Manager Interface Access Pages
