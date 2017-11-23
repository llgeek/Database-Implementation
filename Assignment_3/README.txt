********************* CS 525 Advanced Database Organization -- Fall 2017 ****************

==============================================================================================
||								Programming Assignment 3: Record Manager					||
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
	Compile: make all (or simply input: make)
 	Run: ./test_assign3
 	Clean: make clean
 	Note: make all will compile both test_assign3_1.c and test_expr.c file.

 	The code has also alreadly been put in 'lchen96' folder in the Fourier server. You can directly test the code there as well.
==============================================================================================



*************** Design Ideas and Implementations ************************
* The goal of assignment 3:
	The goal of this assignment is to implement a simple record manager to handle tables with a fixed schema, so that it supports records insertion, deletion, updating and scanning. The scan is associated with a search condition and should only return records that match the search condition. Each table will be stored in a separate page file and the record manager will access the page of the file through the buffer manager implemented in the last assignment.


* Introduction:
	In this assignment, we implement all the interfaces declared in record_mgr.h file and our program passed all the test cases. We use stdlib.h, stdio.h, string.h header files in record_mgr.c file. 
	Bellow we will introduce our design and implementation ideas for each function.


* Design idea and implementations:
To realize all the implementations of the record manager, we need to declare some data structures to support functions like bookkeeping and scanning handling, which are all used as mgmtData. I will introduce all these data structures one by one:

typedef struct RM_TableInfo {
	BM_BufferPool *bm;
	int validTuple; //number of valid tuples in the table //getNumTuples()
	int totalTuple;		//number of total tuples, including invalid tuples
} RM_TableInfo;
This struct stores the bookkeeping information used as mgmtData for RM_TableData. bm stores the buffer manager location. validTuple stores the number of valid tuples and totalTuple stores the total number of tuples, including valid and invalid ones. The reason for keeping these two is that we will use tombstone technicques for record deletion, which means we will keep one byte to for each record to represent whether it's deleted or not. This will definitely increase our record manager performance when deleting the records. 


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
This struct is used as the bookkeeping information for scan handling and will be stored as the mgmtData in RM_ScanHandle. It mainly keeps all the information we will use in next() function. Startscan() function will initialize all the information needed in ScanHandle. More specifically, ScanHandle stores the current page number and current slot number for the scanning, and the scanning condition. Other fields are used to help move the scan pointer to next slot or next page.


The other two struct (BM_FrameHandle and BM_MgmtData) is directly copied from the buffer_mgr.c file, which is declared in our last assignment. These two struct will be used in creating the table stage to initialize the buffer manager. 


Now let's discuss about the implementatioins for each function declared in record_mgr.h file.



1. RC initRecordManager (void *mgmtData):
	This function is used to initialize the record manager. Actually these is no need to do anything here. We just print a welcome sentence. All the initializations will be implemented in createTable function.


2. RC shutdownRecordManager ():
	Similarly to initRecordManager function, since we did no initialization in previous function, there would be also no need to do any resource release implementions here. All shutdown operations will be implemented in closeTable and deleteTable function.


3. RC createTable (char *name, Schema *schema):
	This function is used to create a table, based on the table name and schema declared. We will create a new page to store all the information about the table schema. We assume one page size is enough to store all the schema information, including the number of attributes, attribute data types, attribute length, number of key attributes and their corresponding index. Since the order we store above mentioned information of the schema will affect how we read these information from the page file in openTable function, we have to carefully manage that so we can load them sucessfully. More specifially, we create a new page and pin it to the buffer manager. We store all the schema information to this page and mark this page as dirty. Then we call the shutdownBufferPool to enforce the buffer manager write all these information back to the disk file. 


4.  RC openTable (RM_TableData *rel, char *name):
	This function is used to open the table from the disk file. More specifically, it will load all the information about the table, including all the schema informatioin from the disk. This function can be viewed as a reverse procedure compared with createTable function. The order we store the schema information should be kept when we read them from the file. About the implementation, we firstly create a page and pin the page with 0 index(since the first page is used to store the schema information). Then we load it to the buffer manager and read these data back to initialize the rel parameter. Just make sure everything is exactly the same with the implementation of createTable, so that the loaded information is the same with what we stored in the page file.


5. RC closeTable (RM_TableData *rel):
	This function is used to close the table we created. We need to release all the resources we allocated in the openTable functioin here.


6. RC deleteTable (char *name):
	This function is used to destroy the table file we created in createTable function. Just calling the destroyPageFile function we implemented in storage_mgr.c file is enough.


7. int getNumTuples (RM_TableData *rel):
	This function is used to retrieve the number of tuples stored in the table. Since we keep track of the validTuple in RM_TableInfo, so we can directly return this variable to get the number of tuples.


8. RC insertRecord (RM_TableData *rel, Record *record):
	This function is used to insert one record into the table. Parameter record keeps the information needed about the record to be inserted (its data field). To insert one record, we need firstly find the page and slot number to insert. Firstly we get the total number of tuples, including invaild tuples here. Then based on the PAGE_SIZE and each record size (note here, each record size should be 2 more character than the value returnning from getRecordSize function, cause 1 character is used to store the tombstone flag, and another is used to store the terminator character), we can get total number records each page can support. Note here, we use unspanned technicque to store the record here. Now according to the total number of tuples, and number of records each page can support, we can determine the page number and slot number for the record to insert. Note here, we need to carefully handle with the edge case when we need to increase a new page to insert the record. Another thing here is that, we use the page 0 to store the schema information, so here page index should start from 1 for record insertion. After getting the page and slot number, we assign them to record, and copy the record data field to the desired space in page we just find according to the page and slot number. Then we write the inserted record back to the disk. Don't forget to increment the totalTuple and validTuple here. 


9. RC deleteRecord (RM_TableData *rel, RID id):
	This function is used to delete one record from the table based on the RID information. I use the tombstone to indicate whether one record is deleted, instead of truly deleting it from the table. This can improve our record manager's performance. We just find the page number and location about the record to be deleted, and mark the first character as '1', which indicating it's deleted. This modification does not need to write back to the disk immediately. We adopt the lazy updating for deletion here. Then we decrement the validTuple.



10. RC updateRecord (RM_TableData *rel, Record *record):
	This function is used to update the record in the table according to the parameter record. The idea is similar with deleteRecord function. We firstly find the record to be updated based on the RID info stored in record. Then we copy parameter record's data filed to the space in the page and update the desired record's data. Again, we use the lazy update and there is no need to write the update back to disk immediately. Just mark the page is dirty. 


11. RC getRecord (RM_TableData *rel, RID id, Record *record):
	This function is used to get the record according to the RID and store the record data into the parameter record. Idea is similar, finding the desired record according to RID and write the data and RID into record. 


12. RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond):
	This function is used to initialize a scan request. In this function, we need to initialize all the fields in ScanHandle so that it can used in next() function. We just get all the infomation needed and assign them to ScanHandle here. 


13. RC next (RM_ScanHandle *scan, Record *record):
	This function is used to iterate the database to find all tuples satisfying the scanning condition. There are many edge cases we need to consider when implement the next function. Firstly we should check whether the scan reach the end of the database so that we should terminate the scanning. Then we need to check whether we reach the end of one page file, so that we should move to the next page for scanning. Nextly we should check whether current record is marked as deleted, if so we should call next function to move to the next valid tuple. Then we need to check the condition. If the condition is null, just return the next valid tuple we find. Otherwise we need to call evalExpr function to check whether the next tuple satisfy the scan condition. We should only return the next tuple which satisfies the scan condition. 


14. RC closeScan (RM_ScanHandle *scan):
	This function is used to terminate one scanning handle. We need to release all the resouces allocated in startScan function. 


15.int getRecordSize (Schema *schema):
	This function returns the size in bytes of record for a given schema. We should calculate each attribute's size based on its data type and sum them together as oen record's size.  
  
  
16. Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys):
	This function is used to create a new schema and initialize all the fields based on the give variables. I notatice that all given parameters is pre-allocated in test_assign3_1.c file, so there is no need to malloc new space. Just assign the pointer to the new schema. 
  
17. RC freeSchema (Schema *schema):
 	This function will free the memory allocated to schema and schema data. Since schema's data filed is allocated in test function in test_assign3_1.c file and there is no release there. We neeed to free those space here.

  
18. RC createRecord (Record **record, Schema *schema):
    This function is used to allocate memory for one record. Just malloc the space is enough. 


19. RC freeRecord (Record *record):
    This function is used to free the memory allocated to record and record data in createRecord function. Just free those space.


20. RC getAttr (Record *record, Schema *schema, int attrNum, Value **value):
    This funtion is used to get the value of the deisred attribute in the given record. To retrieve the value, we need to firstly offset the pointer position according to the given attribute position and previous attributes' data size. Then starting from the offset stores the value of the desired attribute and we just copy it from the memory and assign it to the parameter value.


21. RC setAttr (Record *record, Schema *schema, int attrNum, Value *value):
    This function is used to set the value for the given attrNum_th attribute in record. The procedure is similar in getAttr. Firstly we need to find the location storing the desired attribute's value. Then based on the offset, we update the attribute value based on the given value parameter.




    
