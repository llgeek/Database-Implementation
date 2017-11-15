#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dberror.h"
#include "expr.h"
#include "tables.h"

#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "record_mgr.h"

/********************************************************
Define the struct RM_tableInfo
used as bookkeeping for mgmtData pointer in RM_TableData
Xiaolin hu
********************************************************/

typedef struct RM_tableInfo
{
	BM_BufferPool *bm;
	int numTuple; //number of tuples in the table //getNumTuples()
} RM_tableInfo;


// table and manager

/********************************************************
 *initRecordManager() 
 *shutdownRecordManager() 
 *Just return rc_ok?
 *Xiaolin hu
 ********************************************************/
extern RC initRecordManager (void *mgmtData){
	//mgmtData-> bm = 0;
	//BM_BufferPool bm = MAKE_POOL();
	return RC_OK;
}

extern RC shutdownRecordManager (){
	return RC_OK;
}

/********************************************************
 *createTable()
 *create the underlying page file
 *create and store information about the schema, free-space, ...
  and so on in the Table Information pages.
 *Xiaolin hu
 ********************************************************/

extern RC createTable (char *name, Schema *schema){

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
	memcpy(newpage+offset, schema->numAttr, sizeof(int));
	offset += sizeof(int);

	//set attriNames
	int attrLens; //length of an attribute
	for (int i = 0; i < schema->numAttr; i++) {

			attrLens = strlen(schema->attrNames[i])+1;	//get the length of an attribute
			memcpy(newpage+offset, schema->attrNames[i], attrLens);
			offset += attrLens;
		}

	//set dataTypes
	for (int i = 0; i < schema->numAttr; i++) {

			memcpy(newpage+offset, schema->dataTypes[i], sizeof(int));
			offset += sizeof(int);

		}

	//set typeLength
	for (int i = 0; i < schema->numAttr; i++) {
			memcpy(newpage + offset, schema->typeLength[i], sizeof(int));
			offset += sizeof(int);
		}

	//set the keySize
	//IS keySzie = # of keys?
	//Thick: set the keySize firstly
	memcpy(newpage + offset, schema->keySize, sizeof(int));
	offset += sizeof(int);

	//set the keyAttrs
	for (int i = 0; i < schema->keySize; i++) {
			memcpy(newpage + offset, schema->keyAttrs[i], sizeof(int));
			offset += sizeof(int);
		}



	//create a pageHanle to store the string
		BM_PageHandle *aPage = MAKE_HANDLE();
	//set the pointer at page 0 of the buffermgr.
		pinPage(bm, aPage, 0);

		aPage -> data = newpage;

	//write the page back to file
		if ((rc = markDirty(bm, aPage))!= RC_OK)
			return rc;
		if ((rc = unpinPage(bm, aPage))!= RC_OK)
			return rc;
		if ((rc = forceFlushPool(bm))!= RC_OK)
			return rc;
		if ((rc = shutdownBufferPool(bm))!= RC_OK)
			return rc;


		free(bm);
		free(aPage);
		free(newpage);

	return rc;

}

/********************************************************
 *openTable()
 *open table from a file
 *copy the schema from file into the table
 *also can use createSchema() -not use in this function
 *Xiaolin hu
 ********************************************************/


extern RC openTable (RM_TableData *rel, char *name){

		//Initialize buffer manager
		BM_BufferPool *bm = MAKE_POOL();

		//Using LRU strategy
		initBufferPool(bm, name, 100, RS_LRU, NULL);

		//set the name of Record Manager
		rel->name = name;

		//initialize RM_tableInfo
		RM_tableInfo *tableInfo = (RM_tableInfo*)malloc(sizeof(RM_tableInfo));

		tableInfo -> bm = bm;
		tableInfo -> numTuple = 0;

		//assign mgmtData to RM_TableData
		rel -> mgmtData = tableInfo;

		//create a pageHanle to store the string
		BM_PageHandle *aPage = MAKE_HANDLE();
		//set the pointer at page 0 of the buffermgr.
		pinPage(bm, aPage, 0);


		//read the schema into RM_TableData
		//initialize the pagefile

		char *newpage = aPage->data;
		int offset = 0;


		Schema *aschema = (Schema*)malloc(sizeof(Schema));


	  //set numAttr
		int numAttr;
		memcpy(numAttr, newpage+offset, sizeof(int));
		aschema -> numAttr = numAttr;
		offset += sizeof(int);

		//set attrNames
		char attrBuff[255];	//need malloc()?
		int attrLens;	// a counter to get the length of each attribute
		aschema -> attrNames=(char **)malloc(sizeof(char*)*numAttr);
		for(int i =0;i<numAttr;i++){
			attrBuff[255]="";
			attrLens=0;
		    while(*(newpage+offset++)!='\0'){

		        attrBuff[attrLens]=*(newpage+offset-1);
		        attrLens++;
		    }

		    aschema -> attrNames[i] = (char *)malloc(attrLens*sizeof(char)+1);
		    memcpy(aschema->attrNames[i],attrBuff,attrLens);  //or attrLens+1?
		 }

		//set dataTypes
		aschema->dataTypes=(DataType *)malloc(sizeof(DataType)*numAttr);
		for(int i =0;i<numAttr;i++){

			aschema->dataTypes[i]=(int)malloc(sizeof(int));
		    aschema->dataTypes[i] = *((int *)(newpage+offset));
		    offset+=sizeof(int);

		}

		//set typeLength
		aschema->typeLength=(int *)malloc(sizeof(int *)*numAttr);
		for(int i =0;i<numAttr;i++){

			aschema->typeLength[i]=*((int *)(newpage+offset));
		    offset+=sizeof(int);
		}

		//set keysize firstly
		int keySize = *((int *)(newpage+offset));
		aschema->keySize= keySize;
		offset+=sizeof(int);

		//set keyArrtibutes
		aschema->keyAttrs=(int *)malloc(sizeof(int *)*keySize);
		    for(int i =0;i<aschema->keySize;i++)                                       //allocate the space to store the number attributes
		    {
		        aschema->keyAttrs[i]=*((int *)(newpage+offset));
		        offset+=sizeof(int);
		    }


		//store schema into table

		rel->schema = aschema;

		unpinPage(bm, aPage);

		free(newpage);

		return RC_OK;
}
/********************************************************
 *closeTable()
 *using freeSchema()
 *Xiaolin hu
 ********************************************************/
extern RC closeTable (RM_TableData *rel){
	RM_tableInfo *tableInfo = (RM_tableInfo *)rel->mgmtData;

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
extern RC deleteTable (char *name){
	return destroyPageFile(name);
}
/********************************************************
 *getNumTuples()
 *return the tuple numbers in mgmtData
 *Xiaolin hu
 ********************************************************/
extern int getNumTuples (RM_TableData *rel){

		//return the tuple numbers
		RM_tableInfo info = rel-> mgmtData;
		return info ->numTuple;
}
