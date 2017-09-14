#include "storage_mgr.h"
#include "dberror.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define FirstBlock 0;

int main () {
  rc readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	  FILE *ReadFile;
	
	  if(!fHandle){
		  return RC_FILE_NOT_FOUND;
	  }
	
	  if (pageNum < 0 || pageNum > fHandle->totalNumPages) {
		  return RC_NON_EXISTING_PAGE;
	  }
	
	  ReadFile = fopen(fHandle->fileName, "r");
	
	  if(!ReadFile) {
		  return RC_FILE_READ_FAILED;
	  }
	
	  else {
	
		  if(fseek(ReadFile, (pageNum * PAGE_SIZE), SEEK_SET)) {
			  fread(memPage, sizeof(char), PAGE_SIZE, ReadFile);
	  	} 
		  else {
			  return RC_READ_NON_EXISTING_PAGE; 
		  }
		
		  fHandle->curPagePos = ftell(ReadFile); 
		
		  fclose(ReadFile);
		  return RC_OK;
	  }
	
  }

  int getBlockPos(SM_FileHandle *fhandle) {
	  	return(fHandle->curPagePos);
  }


  RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
		
	  RC =  readBlock(0, fHandle, memPage);
	  printError(RC);
	  return RC;
  }

  RC readPreviousBlock(SM_FileHandle *fHandle, SM_PageHandle memPage) {
	
	  RC = readBlock((fHandle->curPagePos - 1), fHandle, memPage);
	  printError(RC);
	  return RC;
  } 

  RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	  RC = readBlock(fHandle->curPagePos, fHandle, memPage);
	  printError(RC)
	  return RC;
  }


  RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	
	  RC = readBlock((fHandle->curPagePos + 1), fHandle, memPage);
	  printError(RC);
	  return RC;
  }

  RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	  RC = readBlock((fHandle->totalNumPages-1)), fHandle, memPage);
	  printError(RC);
	  return RC;
  } 
}



/***
***** author: Linlin Chen
***** email: lchen96@hawk.iit.edu
***/

/**
 * Write pages to disk using absolute position
 * @param  pageNum number of pages to be written
 * @param  fHandle file handler to be written to
 * @param  memPage page handle to be written from
 * @return Error Code         
 */
RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage) {
	int fileseeker = 0;

	/**
	 *  checks the validity of the pageNum 
	 *  Whether it's negative or exceed the maximal number of pages for this file 
	**/
	if (pageNum <= 0 || pageNum > (fHandle->totalNumPages) ) {
		return RC_WRITE_FAILED;
	}

	/** seek  file pointer*/
	fileseeker = fseek(fHandle->mgmtInfo, (pageNum+1)*PAGE_SIZE*sizeof(char), SEEK_SET); /* seeks file write pointer to the pagenumber given by the user */

    if (fileseeker == 0){
        fwrite(memPage, sizeof(char), PAGE_SIZE, fHandle->mgmtInfo); /* writes data from the memory block pointed by memPage to the file. */
        fHandle->curPagePos = pageNum;

        return RC_OK;
    }
    else{
        return RC_WRITE_FAILED;
    }

}



/**
 * Write pages using current position
 * @param  fHandle [description]
 * @param  memPage [description]
 * @return         [description]
 */
RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage) {
	return writeBlock (fHandle->curPagePos, fHandle, memPage); 
}



/**
 * Increase the number of pages in the file by one
 * @param  fHandle [description]
 * @return         [description]
 */
RC appendEmptyBlock (SM_FileHandle *fHandle) {

}






/**
 * Increase the size to numberOfPages in the file size has less than numbnerOfPages pages
 * @param  numberOfPages [description]
 * @param  fHandle       [description]
 * @return               [description]
 */
RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle) {
	if (fHandle->totalNumPages < numberOfPages){
		 /* calculates number of pages required to meet the required size of the file */
		int numPages = numberOfPages - fHandle->totalNumPages;
        int i;
        for (i=0; i < numPages; i++){
			appendEmptyBlock(fHandle); /* increases the size of the file to required size by appending required pages. */
		}
    }
    return RC_OK;
}



