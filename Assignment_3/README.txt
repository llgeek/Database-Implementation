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
	Compile: make test_assign3 (or simply input: make)
 	Run: ./test_assign3
 	Clean: make clean

 	The code has also alreadly been put in 'lchen96' folder in the Fourier server. You can directly test the code there as well.
==============================================================================================



*************** Design Ideas and Implementations ************************
* The goal of assignment 3:
	The goal of this assignment is to implement a simple record manager that allows navigation through records, and inserting and deleting records

* Design idea and implementations:
//Add here all the additional structures defined in our file


1. extern RC initRecordManager (void *mgmtData);


2. extern RC shutdownRecordManager ();


3. extern RC createTable (char *name, Schema *schema);


4. extern RC openTable (RM_TableData *rel, char *name);


5. extern RC closeTable (RM_TableData *rel);


6. extern RC deleteTable (char *name);


7. extern int getNumTuples (RM_TableData *rel);


8. extern RC insertRecord (RM_TableData *rel, Record *record);


9. extern RC deleteRecord (RM_TableData *rel, RID id);


10. extern RC updateRecord (RM_TableData *rel, Record *record);


11. extern RC getRecord (RM_TableData *rel, RID id, Record *record);


12. extern RC startScan (RM_TableData *rel, RM_ScanHandle *scan, Expr *cond);


13. extern RC next (RM_ScanHandle *scan, Record *record);


14. extern RC closeScan (RM_ScanHandle *scan);


15. extern int getRecordSize (Schema *schema);
	this function returns the size in bytes of record for a given schema.
  
  
16. extern Schema *createSchema (int numAttr, char **attrNames, DataType *dataTypes, int *typeLength, int keySize, int *keys);
	this function will memory to the schema and schema data.
  
17. extern RC freeSchema (Schema *schema);
 	this function will free the memory allocated to schema and schema data.
  
18. extern RC createRecord (Record **record, Schema *schema);
    this function will allocate memory to record and record data.

19. extern RC freeRecord (Record *record);
    this function will free the memory allocated to record and record data.

20. extern RC getAttr (Record *record, Schema *schema, int attrNum, Value **value);
    this funtion will get the offset value of attributes.

21. extern RC setAttr (Record *record, Schema *schema, int attrNum, Value *value);
    this function will set the offset value of attributes.
