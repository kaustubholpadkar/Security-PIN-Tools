/*
 * Copyright 2002-2019 Intel Corporation.
 * 
 * This software is provided to you as Sample Source Code as defined in the accompanying
 * End User License Agreement for the Intel(R) Software Development Products ("Agreement")
 * section 1.L.
 * 
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */


#include "pin.H"
#include <iostream>
#include <fstream>
#include <map>

using std::hex;
using std::cerr;
using std::string;
using std::ios;
using std::endl;
using std::map;
/* ===================================================================== */
/* Names of malloc and free */
/* ===================================================================== */
#if defined(TARGET_MAC)
#define MALLOC "_malloc"
#define FREE "_free"
#else
#define MALLOC "malloc"
#define FREE "free"
#endif

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */
UINT64 mallocCount = 0;        //number of calls made to malloc
UINT64 totalMemorySize = 0;        //total amount of memory allocated 
UINT64 memorySize = 0;     //size to be allocated by malloc
map<THREADID, UINT64> thread_map;
std::ofstream TraceFile;
PIN_LOCK lock1, lock2;
/* ===================================================================== */
/* Commandline Switches */
/* ===================================================================== */

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "malloctrace.out", "specify trace file name");

/* ===================================================================== */


/* ===================================================================== */
/* Analysis routines                                                     */
/* ===================================================================== */
 
VOID BeforeMalloc (CHAR * name, ADDRINT size, THREADID threadid)
{
    PIN_GetLock(&lock1, threadid+1);
		mallocCount++;
    thread_map[threadid] = (UINT64) size;
    PIN_ReleaseLock(&lock1);
}

VOID AfterMalloc (ADDRINT ret, THREADID threadid)
{
    PIN_GetLock(&lock1, threadid+1);
		if ( ret != 0 ) {
			totalMemorySize += thread_map[threadid];
		} else {
      
      TraceFile <<  "ret: " << ret << endl; 
    }
    PIN_ReleaseLock(&lock1);
}


/* ===================================================================== */
/* Instrumentation routines                                              */
/* ===================================================================== */
   
VOID Image(IMG img, VOID *v)
{
    // Instrument the malloc() function.

    //  Find the malloc() function.
    RTN mallocRtn = RTN_FindByName(img, MALLOC);
    if (RTN_Valid(mallocRtn))
    {
        RTN_Open(mallocRtn);

        // Instrument malloc() to print the input argument value and the return value.
        RTN_InsertCall(mallocRtn, IPOINT_BEFORE, (AFUNPTR)BeforeMalloc,
                       IARG_ADDRINT, MALLOC, 
                       IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
                       IARG_THREAD_ID,
                       IARG_END);
        RTN_InsertCall(mallocRtn, IPOINT_AFTER, (AFUNPTR)AfterMalloc,
                       IARG_FUNCRET_EXITPOINT_VALUE, IARG_THREAD_ID, IARG_END);

        RTN_Close(mallocRtn);
    }

}

/* ===================================================================== */

/*!
 * Print out analysis results.
 * This function is called when the application exits.
 * @param[in]   code            exit code of the application
 * @param[in]   v               value specified by the tool in the 
 *                              PIN_AddFiniFunction function call
 */
VOID Fini(INT32 code, VOID *v)
{

		TraceFile <<  "Number of calls made to malloc: " << int(mallocCount)  << endl;
		TraceFile <<  "Total amount of memory allocated: " << int(totalMemorySize)  << endl;

    TraceFile.close();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool keeps the track of number of calls made to malloc " << endl <<
            "malloc and, the total amount of memory allocated." << endl << endl;

    cerr << KNOB_BASE::StringKnobSummary() << endl;

    return -1;
}   

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
    // Initialize pin & symbol manager
    PIN_InitSymbols();
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }
    
    // Write to a file since cout and cerr maybe closed by the application
    TraceFile.open(KnobOutputFile.Value().c_str());
    // TraceFile << hex;
    // TraceFile.setf(ios::showbase);
    
    // Register Image to be called to instrument functions.
    IMG_AddInstrumentFunction(Image, 0);
    PIN_AddFiniFunction(Fini, 0);

    // Never returns
    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
