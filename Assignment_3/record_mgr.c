#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "record_mgr.h"

#define RC_BM_PINNED 18
#define RC_PIN_NOUNUSED	19
#define RC_FRAME_NOT_FIND 20

#define RC_DATATYPE_ERROR 21


/********************************************************
Define the struct RM_tableInfo
used as bookkeeping for mgmtData pointer in RM_TableData
Xiaolin hu
********************************************************/
typedef struct RM_TableInfo {
	BM_BufferPool *bm;
	int validTuple; //number of valid tuples in the table //getNumTuples()
	int totalTuple;		//number of total tuples, including invalid tuples
} RM_TableInfo;



typedef struct ScanHandle {
	int curPage;		//current page of the scanning in table
	int curSlot;		//current slot number of the scanning in table
	int numPage;		//total number of pages
	int numSlot;		//total slots in one page
	int recordSize;		//one record's size
	int recordPerPage; 	//record per page
	Expr *cond;		//scan condition
	BM_PageHandle *pgdata;		//current page's data, used for fast lookup
	BM_BufferPool *bm;			//buffer pool
	RM_TableData *rel;		//table database pointer
} ScanHandle;

/**
 * added from buffer_mgr.c
 */
typedef struct BM_FrameHandle {
	//These two are exactly the struct BM_PageHandle declared in header file
	//Modified by Linlin
    // PageNumber pageNum;
    // char *data;
    BM_PageHandle *pgdata;	//page handle, stored the page num and its data in memory
    
    int fix_count; //starting from 0, store the number of clients using this frame
    bool is_dirty; //true is dirty, false is clean, whether the contents is modified 

    //added by Linlin, used to keep loadtime and usedtime, for FIFO and LRU
    int load_time;	//the time this page is loaded to the frame, used in FIFO 
    int used_time;	//the last time this frame is used, used in LRU
    
} BM_FrameHandle;

typedef struct BM_MgmtData { //The bookkeeping info
    
    //using a BM_FrameHandle and a SM_FileHandle to store the pages and current file info
    BM_FrameHandle *frames; //Actually it's an array of frames
    SM_FileHandle *fileHandle;
    
    int read_times;  //getNumReadIO(), number of read times loading contents from disk to memory
    int write_times;  //getNumWriteIO(), number of write times writing contents from memory to disk

    //added by Linlin Chen, to maintain a mapping between page number and page frames for fast look-ups
    //a page to frame mapping is not realstic, since the size of pages always change
    //int *page2frame;	//an array to store the mapping from page number to frames
    int *frame2page;	//an array to store the mapping from frames to page number

} BM_MgmtData;

// table and manager

/********************************************************
 *initRecordManager() 
 *shutdownRecordManager() 
 *Just return rc_ok?
 *Xiaolin hu
 ********************************************************/
RC initRecordManager (void *mgmtData){
	//mgmtData-> bm = 0;
	//BM_BufferPool bm = MAKE_POOL();
	printf("Welcome to use our record manager system！\n");
	return RC_OK;
}

RC shutdownRecordManager (){
	return RC_OK;
}

/********************************************************
 *createTable()
 *create the underlying page file
 *create and store information about the schema, free-space, ...
  and so on in the Table Information pages.
 *Xiaolin hu
 ********************************************************/

RC createTable (char *name, Schema *schema){

	int rc;
	//Initialize page file
	if ((rc = createPageFile(name)) != RC_OK)
	    return rc;

	//Initialize buffer manager

	BM_BufferPool *bm = MAKE_POOL();

	//Set the num of page in the buffer as 100
	//Using LRU strategy
	initBufferPool(bm, name, 100, RS_LRU, NULL);



	//convert the schema into a string, which will be written back to file
	//assuming the size of schema is less than a page
	char *newpage = (char *)malloc(PAGE_SIZE*sizeof(char));
    memset(newpage, '\0', PAGE_SIZE);

	int offset = 0;

	//convert a schema to string, then write the string into the page
	//set numAttr
	memcpy(newpage+offset, &(schema->numAttr), sizeof(int));
	offset += sizeof(int);

	//set attriNames
	size_t attrLens; //length of an attribute
	for (int i = 0; i < schema->numAttr; i++) {
		attrLens = strlen(schema->attrNames[i])+1;	//get the length of an attribute
		memcpy(newpage+offset, schema->attrNames[i], attrLens);
		//*(newpage + offset + attrLens) = '\0';		//set NULL terminator, in case
		//strcpy(newpage+offset, schema->attrNames[i]);
		offset += attrLens;
	}

	//set dataTypes
	for (int i = 0; i < schema->numAttr; i++) {
		memcpy(newpage+offset, &(schema->dataTypes[i]), sizeof(DataType));
		offset += sizeof(DataType);
	}

	//set typeLength
	for (int i = 0; i < schema->numAttr; i++) {
		memcpy(newpage + offset, &(schema->typeLength[i]), sizeof(int));
		offset += sizeof(int);
	}

	//set the keySize
	//Thick: set the keySize firstly
	memcpy(newpage + offset, &(schema->keySize), sizeof(int));
	offset += sizeof(int);

	//set the keyAttrs
	for (int i = 0; i < schema->keySize; i++) {
		memcpy(newpage + offset, &(schema->keyAttrs[i]), sizeof(int));
		offset += sizeof(int);
	}


	//create a pageHanle to store the string
	BM_PageHandle *aPage = MAKE_PAGE_HANDLE();
	//set the pointer at page 0 of the buffermgr.
	pinPage(bm, aPage, 0);

	aPage -> data = newpage;

	//write the page back to file
	if ((rc = markDirty(bm, aPage))!= RC_OK)
		return rc;
	if ((rc = unpinPage(bm, aPage))!= RC_OK)
		return rc;
	// if ((rc = forceFlushPool(bm))!= RC_OK)
	// 	return rc;
	if ((rc = shutdownBufferPool(bm))!= RC_OK)
		return rc;

	free(newpage);
	free(aPage);
	free(bm);

	return rc;

}

/********************************************************
 *openTable()
 *open table from a file
 *copy the schema from file into the table
 *also can use createSchema() -not use in this function
 *Xiaolin hu
 ********************************************************/


RC openTable (RM_TableData *rel, char *name){

	//Initialize buffer manager
	BM_BufferPool *bm = MAKE_POOL();

	//Using LRU strategy
	initBufferPool(bm, name, 100, RS_LRU, NULL);

	//set the name of Record Manager
	rel->name = (char *) malloc(sizeof(char)*10);
	strcpy(rel->name, name);

	//initialize RM_TableInfo
	RM_TableInfo *tableInfo = (RM_TableInfo *) malloc(sizeof(RM_TableInfo));

	tableInfo->bm = bm;
	tableInfo->validTuple = 0;
	tableInfo->totalTuple = 0;

	//assign mgmtData to RM_TableData
	rel -> mgmtData = tableInfo;

	//create a pageHanle to store the string
	BM_PageHandle *aPage = MAKE_PAGE_HANDLE();
	//set the pointer at page 0 of the buffermgr.
	pinPage(bm, aPage, 0);


	//read the schema into RM_TableData
	//initialize the pagefile
	char *newpage = aPage->data;

	Schema *aschema = (Schema *) malloc(sizeof(Schema));

  	//set numAttr
	int numAttr;
	int offset = 0;
	memcpy(&numAttr, newpage+offset, sizeof(int));
	aschema -> numAttr = numAttr;
	offset += sizeof(int);

	//set attrNames
	char attrBuff[255];	//need malloc()?
	int attrLens;	// a counter to get the length of each attribute
	aschema -> attrNames=(char **)malloc(sizeof(char*)*numAttr);

	for (int i = 0; i < numAttr; i++) { 
		memset(attrBuff, '\0', 255);
		attrLens = 0;
		while(*(newpage + offset) != '\0') { 		//find the terminator, assiging the string (name of attributes)
			attrBuff[attrLens] = *(newpage + offset);
			attrLens ++;
			offset ++;
		}
		aschema -> attrNames[i] = (char *)malloc((attrLens+1)*sizeof(char));
	    //memcpy(aschema->attrNames[i], attrBuff, attrLens);  //or attrLens+1?
	    strcpy(aschema->attrNames[i], attrBuff);
	}

	//set dataTypes
	aschema->dataTypes = (DataType *) malloc(sizeof(DataType) * numAttr);
	for (int i = 0; i < numAttr; i++) {
		//aschema->dataTypes[i] = (int)malloc(sizeof(int));
		//aschema->dataTypes[i] = *((int *)(newpage + offset));
		memcpy(&(aschema->dataTypes[i]), newpage + offset, sizeof(DataType));
		offset += sizeof(DataType);
	}

	//set typeLength
	aschema->typeLength = (int *) malloc(sizeof(int) * numAttr);
	for (int i = 0; i < numAttr; i++) {
		//aschema->typeLength[i] = *((int *) (newpage + offset));
		memcpy(&(aschema->typeLength[i]), newpage + offset, sizeof(int));
		offset += sizeof(int);
	}

	//set keysize firstly
	//int keySize = *((int *)(newpage+offset));
	//aschema->keySize= keySize;
	memcpy(&(aschema->keySize), newpage + offset, sizeof(int));
	offset += sizeof(int);

	//set keyArrtibutes
	aschema->keyAttrs = (int *)malloc(sizeof(int) * aschema->keySize);
	for(int i = 0; i < aschema->keySize; i++) {        //allocate the space to store the number attributes
	   	//aschema->keyAttrs[i] = *((int *) (newpage + offset));
	   	memcpy(&(aschema->keyAttrs[i]), newpage + offset, sizeof(int));
	    offset += sizeof(int); 
	}

	//store schema into table

	rel->schema = aschema;
	unpinPage(bm, aPage);
	free(aPage);
	return RC_OK;
}

/********************************************************
 *closeTable()
 *using freeSchema()
 *Xiaolin hu
 ********************************************************/
RC closeTable (RM_TableData *rel){
	RM_TableInfo *tableInfo = (RM_TableInfo *)rel->mgmtData;
	BM_BufferPool *bm = (BM_BufferPool *)tableInfo->bm;

	freeSchema(rel->schema);
	shutdownBufferPool(bm);
	free(tableInfo);
	return RC_OK;

}
/********************************************************
 *deleteTable()
 *delete the corresponding pagefile
 *Xiaolin hu
 ********************************************************/
RC deleteTable (char *name){
	return destroyPageFile(name);
}
/********************************************************
 *getNumTuples()
 *return the tuple numbers in mgmtData
 *Xiaolin hu
 ********************************************************/
int getNumTuples (RM_TableData *rel) {
	//return the tuple numbers
	RM_TableInfo* info = (RM_TableInfo *) rel-> mgmtData;
	return info ->validTuple;
}


int getTotalTuples (RM_TableData *rel) {
	RM_TableInfo* info = (RM_TableInfo *) rel->mgmtData;
	return info->totalTuple;
}



/**
 *  Start the implementions of handling records and scans
 *  Linlin Chen
 *  lchen96@hawk.iit.edu
 *  A20348195
 */

/**
 * insert a record to the table
 * @param  rel:    Table data
 * @param  record: the record to be inserted
 * @return        Error code
 */
//note page num should start from 1, since page 0 stores the schema information!
RC insertRecord (RM_TableData *rel, Record *record) {
	//get buffer and file info according to rel
	RM_TableInfo *rm = (RM_TableInfo *) rel->mgmtData;
	BM_BufferPool *bm = rm->bm;
	//allocate a new page to store record
	BM_PageHandle *page = (BM_PageHandle *) malloc(sizeof(BM_PageHandle));

	int recordsize;		//store one record's size
	if ((recordsize = getRecordSize(rel->schema)) == -1)		//error in getting schema's record size
		return  RC_FILE_NOT_FOUND;
	//I will use one int value to indicate whether this record is deleted or not.
	recordsize += (sizeof(char) + sizeof(int));		//char for '\n', int for tombstone, marking dirty when record is deleted

	//total records in existing table
	////should return the total number of valid and invalid records
	///because invalid record still comsumes the space in page
	int totaltuples = getTotalTuples(rel);

	//how many records can be stored in one page, use unspanned records
	int recordperpage = PAGE_SIZE / recordsize;	
	
	//the new pageNum will be used for insertion
	int pageNum = totaltuples / recordperpage + 1;	//skip page 0, since it stores schema info

	//offset from the start location of this page
	int slot = 0;		
	//last page has been exhausted, assigning a new page
	if (totaltuples % recordperpage == 0) {
		slot = 0;
	} else {	//still can use the last page to insert the record
		//offset equals the record number in this page times the record size
		slot = (totaltuples % recordperpage)*recordsize;
	}


	//assigning value to the page
	page->pageNum = pageNum;
	//pin the page of pageNum
	pinPage(bm, page, pageNum);

	record->id.page = pageNum;
	record->id.slot = slot;
	

	char * tempbuf = (char *) malloc(sizeof(recordsize));
	*((int *) tempbuf) = 0;		//set the tombstone as 0, indicating it's undeleted
	memcpy(tempbuf + sizeof(int), record->data, recordsize - sizeof(int) - sizeof(char));
	*((char *) tempbuf + recordsize - sizeof(char)) = '\0';

	//insert the record to the page offset the slot size
	memcpy(page->data + slot, tempbuf, recordsize);

	//mark the page is dirty, cache the modification and will not write back immediately
	//write the page content back to the file
	markDirty(bm, page);
	forcePage(bm, page);
	unpinPage(bm, page);

	rm->validTuple ++;
	rm->totalTuple ++;

	free(tempbuf);

	return RC_OK;
}


/**
 * This funciton is used to delete one record according to the id info
 * @param  rel relation database
 * @param  id  record id to be deleted
 * @return     error code
 */
RC deleteRecord (RM_TableData *rel, RID id) {
	RM_TableInfo *rm = (RM_TableInfo *) rel->mgmtData;
	BM_BufferPool *bm = rm->bm;

	BM_PageHandle *page = (BM_PageHandle *) malloc(sizeof(BM_PageHandle));

	//get the page stores the record with RID id
	pinPage(bm, page, id.page);

	//starting int stores the value indicating whether this record is valid
	//1 indicates this record is invalid, 0 indicates valid
	*((int *)(page->data) + id.slot) = 1;		

	//mark the page is dirty, cache the modification and will not write back immediately
	markDirty(bm, page);
	unpinPage(bm, page);

	rm->validTuple -= 1;

	return RC_OK;
}


/**
 * This function is used to update one record's data according to the given record
 * @param  rel    relation database
 * @param  record record used to update
 * @return        error code
 */
RC updateRecord (RM_TableData *rel, Record *record) {
	RM_TableInfo *rm = (RM_TableInfo *) rel->mgmtData;
	BM_BufferPool *bm = rm->bm;

	BM_PageHandle *page = (BM_PageHandle *) malloc(sizeof(BM_PageHandle));

	//pin page according to record's RID id info
	pinPage(bm, page, record->id.page);

	int recordsize = getRecordSize(rel->schema);
	//update the record value
	memcpy((page->data) + record->id.slot + sizeof(int), record->data, recordsize);

	//mark the page is dirty, cache the modification and will not write back immediately
	markDirty(bm, page);
	unpinPage(bm, page);

	return RC_OK;
}

/**
 * get the record with id as RID, and store the record in *record
 * @param  rel:		table data    
 * @param  id     id num
 * @param  record: record to be stored
 * @return        error code
 */
RC getRecord (RM_TableData *rel, RID id, Record *record) {
	RM_TableInfo *rm = (RM_TableInfo *) rel->mgmtData;
	BM_BufferPool *bm = rm->bm;

	BM_PageHandle *page = (BM_PageHandle *) malloc(sizeof(BM_PageHandle));

	//pin page according to record's RID id info
	pinPage(bm, page, id.page);

	int recordsize = getRecordSize(rel->schema);

	//check whether the given record is already deleted
	if (*(int *)((page->data) + id.slot) == 1) {
		printf("ERROR: Record to be retrived is invalid and has been deleted!\n");
	}
	//copy the record data
	memcpy(record->data, (page->data) + id.slot + sizeof(int), recordsize);

	//assigning RID info
	record->id.slot = id.slot;
	record->id.page = id.page;

	unpinPage(bm, page);

	return RC_OK;

}

/**
 * This function is used to initialize a scan handler for database scanning
 * @param  rel  	relation database
 * @param  scan 	scan handler to be updated
 * @param  cond 	scan condition
 * @return     		error code
 */
RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond) {
	RM_TableInfo * rm = (RM_TableInfo *) rel->mgmtData;
	BM_BufferPool *bm = (BM_BufferPool *) rm->bm;
	BM_MgmtData *bmmgmt = (BM_MgmtData *) bm->mgmtData;

	BM_PageHandle *pgdata = (BM_PageHandle *) malloc(sizeof(BM_PageHandle));
	pinPage(bm, pgdata, 1);

	ScanHandle *sh = (ScanHandle *) malloc (sizeof(ScanHandle));

	int recordsize;		//store one record's size
	if ((recordsize = getRecordSize(rel->schema)) == -1)		//error in getting schema's record size
		return  RC_FILE_NOT_FOUND;
	recordsize += (sizeof(char) + sizeof(int));		//char for '\n', int for tombstone, marking dirty when record is deleted

	//how many records can be stored in one page, use unspanned records
	int recordperpage = PAGE_SIZE / recordsize;	


	sh->curPage = 1;		//current page of the scanning in table, starting from 1
	sh->curSlot = 0;		//current slot number of the scanning in table
	sh->numPage = bmmgmt->fileHandle->totalNumPages;		//total number of pages
	sh->numSlot = recordperpage * recordsize;		//total slots in one page
	sh->recordSize = recordsize;	//one record size
	sh->recordPerPage = recordperpage;	//record per page
	sh->cond = cond;
	sh->pgdata = pgdata;
	sh->bm = bm;
	sh->rel = rel;

	unpinPage(bm, pgdata);

	scan->rel = rel;
	scan->mgmtData = sh;



	return RC_OK;
}


/**
 * this function is used to iterate the database to find all tuples satisfying the scanning condition
 * @param  scan   	scan handler
 * @param  record  	returning record, directly update it
 * @return        	error code
 */
RC next (RM_ScanHandle *scan, Record *record) {
	ScanHandle *sh = (ScanHandle *) scan->mgmtData;		//casting


	//no more tuples, scan is complete
	if (sh->curPage == sh->numPage-1 && sh->curSlot == sh->numSlot) {
		return RC_RM_NO_MORE_TUPLES;
	} 
	//scan handler reach the end of one page, move to the next page
	if (sh->curSlot == sh->numSlot) {
		sh->curPage ++;
		sh->curSlot = 0;
		pinPage(sh->bm, sh->pgdata, sh->curPage);
		unpinPage(sh->bm, sh->pgdata);
	}

	//current record is marked as deleted, then skip this record
	if (*(int *)(sh->pgdata->data + sh->curSlot) == 1) {
		sh->curSlot += sh->recordSize;
		next(scan, record);
	}
	//condition is NULL, returning all the tuples back
	if (sh->cond == NULL) {
		record->id.page = sh->curPage;
		record->id.slot = sh->curSlot;
		//assign the record data value
		memcpy(record->data, sh->pgdata->data + sh->curSlot + sizeof(int), sh->recordSize - sizeof(int));
		sh->curSlot += sh->recordSize;
		scan->mgmtData = sh;
		return RC_OK;
	} 
	//scanning with condition, need to check the condition
	else {
		record->id.page = sh->curPage;
		record->id.slot = sh->curSlot;
		//assign the record data value
		memcpy(record->data, sh->pgdata->data + sh->curSlot + sizeof(int), sh->recordSize - sizeof(int));
		sh->curSlot += sh->recordSize;
		scan->mgmtData = sh;

		Value *result = (Value *) malloc(sizeof(Value));
		//check whether this record satisfy the scan condition
		evalExpr(record, sh->rel->schema, sh->cond, &result);
		//if statisfy, directly return this record
		if (result->v.boolV)
			return RC_OK;
		//otherwise, iterate the next untill finding a one or till the end
		else
			next(scan, record);
	}
	return RC_OK;
}



/**
 * close the scan handler
 * @param  scan  scan handler to be closed
 * @return      error code
 */
RC closeScan (RM_ScanHandle *scan) {
	ScanHandle *sh = (ScanHandle *) scan->mgmtData;
	free(sh->pgdata);
	free(sh);
	return RC_OK;
}
/**
 *  End the implementions of handling records and scans
 *  Linlin Chen
 *  lchen96@hawk.iit.edu
 *  A20348195
 */



/**
 *
 *  dealing with schemas
 *	Start Implementation
 *
**/
/**
 *
 * Module	:getRecordSize
 * Description: It returns the size in bytes of record for a given schema
 * If exists error in dealing with the schema, return -1
 **/

int getRecordSize (Schema *schema){

	//Validating Schema
	if(!schema) {
		return -1;
	}
	
    int recordSize = 0;

    for(int i = 0; i < schema->numAttr; i++){
    	switch (schema->dataTypes[i]){
    		//data type is int
    		case DT_INT:
    			recordSize += sizeof(int);
    			break;
    		//data type is float
    		case DT_FLOAT:
    			recordSize += sizeof(float);
    			break;
    		//data tpe if bool
    		case DT_BOOL:
    			recordSize += sizeof(bool);
    			break;
    		//data type is string
    		case DT_STRING:
    			recordSize += schema->typeLength[i] * sizeof(char);
    			break;
    		// cannot find such data type
    		// return -1
    		default:
    			printf("Datatype Error!\n");
    			return -1;
    	}
    }

    return recordSize;

}//getRecordSize

/*
 *
 * Module	:createSchema
 * Description: It creates new schema
 *
 */

//Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys){
//
//	Schema *schema = (Schema *)malloc(sizeof(Schema)); 
//
//	//assigning value to the schema
//	schema->numAttr = numAttr;
//
//	//to keep the parameters safe, we should malloc new space instead of assigning parameters' pointer here
//	//allocating space
//	schema->attrNames = (char **)malloc(sizeof(char)*numAttr);
//	schema->dataTypes = (DataType *) malloc(sizeof(DataType)*numAttr);
//	schema->typeLength = (int *) malloc(sizeof(int)*numAttr);
//
//	//assigning values
//	for (int i = 0; i < numAttr; i++) {
//		schema->attrNames[i] = (char *) malloc(sizeof(char)*10);
//		memset(schema->attrNames[i], '\0', 10);
//		strcpy(schema->attrNames[i], attrNames[i]);
//
//		schema->dataTypes[i] = dataTypes[i];
//		schema->typeLength[i] = typeLength[i];
//	}
//	
//	schema->keySize = keySize;
//
//	schema->keyAttrs = (int *) malloc(sizeof(int)*keySize);
//	for (int i = 0; i < keySize; i++)
//		schema->keyAttrs[i] = keys[i];
//
//	return schema;
//
//}//*createSchema

Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys){
    Schema *schema = (Schema *)malloc(sizeof(Schema));
    schema->numAttr = numAttr;
    schema->attrNames = attrNames;
    schema->dataTypes = dataTypes;
    schema->typeLength = typeLength;
    schema->keySize = keySize;
    schema->keyAttrs = keys;
    return schema;
}


/*
 *
 * Module	:freeSchema
 * Description: It free the schema
 *
 */

RC freeSchema (Schema *schema){
	//should free all space allocated in createSchema function
	for (int i = 0; i < schema->numAttr; i++) {
		free(schema->attrNames[i]);
	}
	free(schema->attrNames);
	free(schema->dataTypes);
	free(schema->typeLength);
	free(schema->keyAttrs);

	free(schema);

	//free schema memory
	return	RC_OK;

}//freeSchema



/* Dealing with records and attribute values */

// Allocates memory for the record
RC createRecord (Record **record, Schema *schema){
	//allocating the space for record
	*record = (Record *) malloc(sizeof(Record));
	//(*record)->id = (RID *) malloc(sizeof(RID));
	//data size should equal to one record size in schema
	(*record)->data = (char *) malloc(getRecordSize(schema));
	
   	 return RC_OK;
}

//Freeing the record
RC freeRecord (Record *record){
    /* free the memory space allocated to record and its data */
	 
    //Free the data of record memory
    free(record->data);
	
	//free ths space for id
	//free(record->id);

    //Free the record memorry
    free(record);

    //Returning RC_OK if sucessful
    return RC_OK;
}


/**
 * This function is used to get the position offset of the attrNum_th attributes
 * This function will be used in setAttr and getAttr functions
 * @param  schema  schema of the table
 * @param  attrNum offset of the attribute num
 * @return         offset position
 */
int offsetAttr(Schema *schema, int attrNum) {
	int offset = 0;
	for (int i = 0; i < attrNum; i++) {
		if (schema->dataTypes[i] == DT_INT) 
			offset += sizeof(int);
		else if (schema->dataTypes[i] == DT_STRING)
			offset += schema->typeLength[i] * sizeof(char);
		else if (schema->dataTypes[i] == DT_FLOAT)
			offset += sizeof(float);
		else if (schema->dataTypes[i] == DT_BOOL)
			offset += sizeof(bool);
		else {
			//finding error 
			//no such data type allowed
			printf("DATA TYPE ERROR!\n");
			return -1;
		}
	}
	return offset;
}


//Getting the value of the attrnum_th attribute in table
//Note attrNum should start from 1 here
RC getAttr (Record *record, Schema *schema, int attrNum, Value **value){
	//allocating space for value result
	Value *tmpv = (Value *)malloc(sizeof(Value));	
	//value type assigned according to schema
	tmpv->dt = schema->dataTypes[attrNum];

	int offset = offsetAttr(schema, attrNum);

	//-1 means we get a data type which is not defined in the schema
	if (offset == -1) {
		printf("Error: data type not allowed!\n");
		return RC_DATATYPE_ERROR;
	}
    
    //set the value for value of attrNum_th attributes
    int typelength = 0;
    switch(schema->dataTypes[attrNum]) {
    	case DT_INT:		//data type int
    		memcpy(&(tmpv->v.intV), record->data + offset, sizeof(int));
    		break;

    	case DT_STRING:		//data type string
    		typelength = schema->typeLength[attrNum];
    		tmpv->v.stringV = (char *)malloc(sizeof(char)*(typelength+1));	//reserve 1 space for '\0'
    		strncpy(tmpv->v.stringV, record->data + offset, typelength);
    		tmpv->v.stringV[typelength] = '\0';		// add the edding character for the string
    		break;

    	case DT_FLOAT:		//data type float
    		memcpy(&(tmpv->v.floatV), record->data + offset, sizeof(float));
    		break;

    	case DT_BOOL:		//data type bool
    		memcpy(&(tmpv->v.boolV), record->data + offset, sizeof(bool));
    		break;
    	default:		//finding error, no such data type allowed
    		printf("Error: data type not allowed!\n");
			return RC_DATATYPE_ERROR;
    }
    *value = tmpv;
    return RC_OK;

}

//see the value for attrNum_th attributes, according to the value
//attrNum should start from 1, instead of 0, here
RC setAttr (Record *record, Schema *schema, int attrNum, Value *value) {

	int offset = offsetAttr(schema, attrNum);

	//-1 means we get a data type which is not defined in the schema
	if (offset == -1) {
		printf("Error: data type not allowed!\n");
		return RC_DATATYPE_ERROR;
	}
	char *data = record->data + offset;		//get the start location to be written

    //set the value for record of attrNum_th attributes
    int typelength = 0;
    switch(schema->dataTypes[attrNum]) {
    	case DT_INT:		//data type int
    		memcpy(data, &(value->v.intV), sizeof(int));
    		break;

    	case DT_STRING:		//data type string
    		typelength = schema->typeLength[attrNum];
    		strncpy(data, value->v.stringV, typelength);		//no '\0' will be copied
    		break;

    	case DT_FLOAT:		//data type float
    		memcpy(data, &(value->v.floatV), sizeof(float));
    		break;

    	case DT_BOOL:		//data type bool
    		memcpy(data, &(value->v.boolV), sizeof(bool));
    		break;
    	default:		//finding error, no such data type allowed
    		printf("Error: data type not allowed!\n");
			return RC_DATATYPE_ERROR;
    }

    return RC_OK;
}

