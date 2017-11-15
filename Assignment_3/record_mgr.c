#include "dberror.h"
#include "expr.h"
#include "tables.h"

#include "storage_mgr.h"
#include "buffer_mgr.h"
#include "record_mgr.h"

// table and manager
extern RC initRecordManager (void *mgmtData);
extern RC shutdownRecordManager ();
extern RC createTable (char *name, Schema *schema);
extern RC openTable (RM_TableData *rel, char *name);
extern RC closeTable (RM_TableData *rel);
extern RC deleteTable (char *name);
extern int getNumTuples (RM_TableData *rel);
