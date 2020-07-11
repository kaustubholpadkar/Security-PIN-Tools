#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <cctype>
#include <map>
#include "pin.H"
using std::cerr;
using std::string;
using std::endl;


// Command line switches for this tool.
//
// KNOB<ADDRINT> KnobStackBreak(KNOB_MODE_WRITEONCE, "pintool",
//    "stackbreak", "1",
//    "Stop at breakpoint when thread uses this much stack (use with -appdebug_enable");
KNOB<std::string> KnobOut(KNOB_MODE_WRITEONCE, "pintool",
    "o", "stacktool.log",
    "When using -stackbreak, debugger connection information is printed to this file (default stderr)");
KNOB<UINT32> KnobTimeout(KNOB_MODE_WRITEONCE, "pintool",
    "timeout", "0",
    "When using -stackbreak, wait for this many seconds for debugger to connect (zero means wait forever)");


// Virtual register we use to point to each thread's TINFO structure.
//
static REG RegTinfo;


// Information about each thread.
//
struct TINFO
{
    TINFO(ADDRINT base) : _stackBase(base), _max(0), _maxReported(0) {}

    ADDRINT _stackBase;     // Base (highest address) of stack.
    size_t _max;            // Maximum stack usage so far.
    size_t _maxReported;    // Maximum stack usage reported at breakpoint.
    std::ostringstream _os; // Used to format messages.
};

typedef std::map<THREADID, TINFO *> TINFO_MAP;
static TINFO_MAP ThreadInfos;

static std::ostream *Output = &std::cerr;
// static bool EnableInstrumentation = false;
// static bool BreakOnNewMax = false;
// static ADDRINT BreakOnSize = 0;


INT32 Usage()
{
    cerr << "This tool demonstrates the use of extended debugger commands" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}


static VOID OnThreadStart(THREADID tid, CONTEXT *ctxt, INT32, VOID *)
{
    TINFO *tinfo = new TINFO(PIN_GetContextReg(ctxt, REG_STACK_PTR));
    ThreadInfos.insert(std::make_pair(tid, tinfo));
    PIN_SetContextReg(ctxt, RegTinfo, reinterpret_cast<ADDRINT>(tinfo));
    // cerr << "start Thread ID:" << tid << "Max Stack Used:" << tinfo->_max << endl;
}

/*
static VOID OnThreadEnd(THREADID tid, const CONTEXT *ctxt, INT32, VOID *)
{
    TINFO *tinfo = reinterpret_cast<TINFO *>(PIN_GetContextReg(ctxt, RegTinfo));
    cerr << "end Thread ID:" << tid << "Max Stack Used:" << tinfo->_max << endl;

    TINFO_MAP::iterator it = ThreadInfos.find(tid);

    if (it != ThreadInfos.end())
    {
        delete it->second;
        ThreadInfos.erase(it);
    }

}
*/

static ADDRINT OnStackChangeIf(ADDRINT sp, ADDRINT addrInfo)
{   
    TINFO *tinfo = reinterpret_cast<TINFO *>(addrInfo);
    
    // The stack pointer may go above the base slightly.  (For example, the application's dynamic
    // loader does this briefly during start-up.)
    // 
    if (sp > tinfo->_stackBase)
        return 0;
    
    // Keep track of the maximum stack usage.
    //
    size_t size = tinfo->_stackBase - sp;
    if (size > tinfo->_max)
        tinfo->_max = size;
    
    // See if we need to trigger a breakpoint.
    //

		/*
    if (BreakOnNewMax && size > tinfo->_maxReported)
        return 1;
    if (BreakOnSize && size >= BreakOnSize)
        return 1;
		*/
    return 0;
}



static VOID DoBreakpoint(const CONTEXT *ctxt, THREADID tid)
{
    TINFO *tinfo = reinterpret_cast<TINFO *>(PIN_GetContextReg(ctxt, RegTinfo));

    // Keep track of the maximum reported stack usage for "stackbreak newmax".
    //
    size_t size = tinfo->_stackBase - PIN_GetContextReg(ctxt, REG_STACK_PTR);
    if (size > tinfo->_maxReported)
        tinfo->_maxReported = size;

//    ConnectDebugger();  // Ask the user to connect a debugger, if it is not already connected.

    // Construct a string that the debugger will print when it stops.  If a debugger is
    // not connected, no breakpoint is triggered and execution resumes immediately.
    //
    tinfo->_os.str("");
    tinfo->_os << "Thread " << std::dec << tid << " uses " << size << " bytes of stack.";
    // PIN_ApplicationBreakpoint(ctxt, tid, FALSE, tinfo->_os.str());
}



static VOID Instruction(INS ins, VOID *)
{
    
    if (INS_RegWContain(ins, REG_STACK_PTR))
    {   
        if (INS_IsSysenter(ins)) return; // no need to instrument system calls
        
        IPOINT where = IPOINT_AFTER;
        if (!INS_IsValidForIpointAfter(ins))
        {   
            if (INS_IsValidForIpointTakenBranch(ins))
            {   
                where = IPOINT_TAKEN_BRANCH;
            }
            else
            {   
                return;
            }
        }
        INS_InsertIfCall(ins, where, (AFUNPTR)OnStackChangeIf, IARG_REG_VALUE, REG_STACK_PTR, IARG_REG_VALUE, RegTinfo, IARG_END);
        
        // We use IARG_CONST_CONTEXT here instead of IARG_CONTEXT because it is faster.
        //
        INS_InsertThenCall(ins, where, (AFUNPTR)DoBreakpoint, IARG_CONST_CONTEXT, IARG_THREAD_ID, IARG_END);
    }


}


// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
  // Write to a file since cout and cerr maybe closed by the application

  std::map<THREADID, TINFO *>::iterator it;
  
  for ( it = ThreadInfos.begin(); it != ThreadInfos.end(); it++ ) {
    *Output << "Thread ID:" << it->first << " | Max Stack Used:" << it->second->_max << endl;  

  }


}


int main(int argc, char *argv[])
{
    PIN_InitSymbols();

    if (PIN_Init(argc, argv)) return Usage();

    if (!KnobOut.Value().empty())
        Output = new std::ofstream(KnobOut.Value().c_str());

//    if (KnobStackBreak.Value())
//    {
//        EnableInstrumentation = true;
//        BreakOnSize = KnobStackBreak.Value();
//    }

    // Allocate a virtual register that each thread uses to point to its
    // TINFO data.  Threads can use this virtual register to quickly access
    // their own thread-private data.
    //
    RegTinfo = PIN_ClaimToolRegister();
    if (!REG_valid(RegTinfo))
    {
        std::cerr << "Cannot allocate a scratch register.\n";
        std::cerr << std::flush;
        return 1;
    }
    PIN_AddThreadStartFunction(OnThreadStart, 0);
//    PIN_AddThreadFiniFunction(OnThreadEnd, 0);
    INS_AddInstrumentFunction(Instruction, 0);
    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    PIN_StartProgram();
    return 0;
}
