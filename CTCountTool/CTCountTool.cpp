
/*! @file
 *  This is an example of the PIN tool that demonstrates some basic PIN APIs 
 *  and could serve as the starting point for developing your first PIN tool
 */

#include "pin.H"
#include <iostream>
#include <fstream>
using std::cerr;
using std::string;
using std::endl;
/* ================================================================== */
// Global variables 
/* ================================================================== */

UINT64 directCTCount = 0;        //number of dynamically executed basic blocks
UINT64 indirectCTCount = 0;
UINT64 otherCTCount = 0;
PIN_LOCK lock1, lock2, lock3;
std::ostream * out = &cerr;

/* ===================================================================== */
// Command line switches
/* ===================================================================== */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,  "pintool",
    "o", "", "specify file name for CTCountTool output");

KNOB<BOOL>   KnobCount(KNOB_MODE_WRITEONCE,  "pintool",
    "count", "1", "count instructions, basic blocks and threads in the application");


/* ===================================================================== */
// Utilities
/* ===================================================================== */

/*!
 *  Print out help message.
 */
INT32 Usage()
{
    cerr << "This tool prints out the number of dynamically executed " << endl <<
            "instructions, basic blocks and threads in the application." << endl << endl;

    cerr << KNOB_BASE::StringKnobSummary() << endl;

    return -1;
}

/* ===================================================================== */
// Analysis routines
/* ===================================================================== */

/*!
 * Increase counter of the executed basic blocks and instructions.
 * This function is called for every basic block when it is about to be executed.
 * @param[in]   numInstInBbl    number of instructions in the basic block
 * @note use atomic operations for multi-threaded applications
 */


VOID docount1 (THREADID threadid)
{
  PIN_GetLock(&lock1, threadid+1);
	directCTCount++;
  PIN_ReleaseLock(&lock1);
}

VOID docount2 (THREADID threadid)
{
  PIN_GetLock(&lock2, threadid+1);
	indirectCTCount++;
  PIN_ReleaseLock(&lock2);
}

VOID docount3 (THREADID threadid)
{
  PIN_GetLock(&lock3, threadid+1);
	otherCTCount++;
  PIN_ReleaseLock(&lock3);
}

/* ===================================================================== */
// Instrumentation callbacks
/* ===================================================================== */

/*!
 * Insert call to the CountBbl() analysis routine before every basic block 
 * of the trace.
 * This function is called every time a new trace is encountered.
 * @param[in]   trace    trace to be instrumented
 * @param[in]   v        value specified by the tool in the TRACE_AddInstrumentFunction
 *                       function call
 */

VOID Instruction(INS ins, void *v)
{

    if (INS_IsDirectControlFlow(ins))
    {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) docount1,
                       IARG_THREAD_ID,
                       IARG_END);
    }
		else if (INS_IsIndirectControlFlow(ins))
		{
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) docount2,
                       IARG_THREAD_ID,
                       IARG_END);				
		}
    else
    {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR) docount3,
                       IARG_THREAD_ID, 
                       IARG_END);
    }

}
/*!
 * Print out analysis results.
 * This function is called when the application exits.
 * @param[in]   code            exit code of the application
 * @param[in]   v               value specified by the tool in the 
 *                              PIN_AddFiniFunction function call
 */
VOID Fini(INT32 code, VOID *v)
{
    *out <<  "Number of direct control flow transfer instructions: " << directCTCount << endl;
		*out <<  "Number of indirect control flow transfer instructions: " << indirectCTCount << endl;
		*out <<  "Number of other control flow transfer instructions: " << otherCTCount << endl;
}
/*!
 * The main procedure of the tool.
 * This function is called when the application image is loaded but not yet started.
 * @param[in]   argc            total number of elements in the argv array
 * @param[in]   argv            array of command line arguments, 
 *                              including pin -t <toolname> -- ...
 */
int main(int argc, char *argv[])
{
    // Initialize PIN library. Print help message if -h(elp) is specified
    // in the command line or the command line is invalid 
    if( PIN_Init(argc,argv) )
    {
        return Usage();
    }
    
    string fileName = KnobOutputFile.Value();

    if (!fileName.empty()) { out = new std::ofstream(fileName.c_str());}

    if (KnobCount)
    {
        // Register Instruction to be called to instrument instructions
        INS_AddInstrumentFunction(Instruction, 0);

        // Register function to be called when the application exits
        PIN_AddFiniFunction(Fini, 0);
    }
    
    /*
    if (!KnobOutputFile.Value().empty()) 
    {
        cerr << "See file " << KnobOutputFile.Value() << " for analysis results" << endl;
    }
    cerr <<  "===============================================" << endl;
    */
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}

/* ===================================================================== */
/* eof */
/* ===================================================================== */
