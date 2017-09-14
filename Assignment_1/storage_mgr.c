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
