
/*! @file
 *  This is an example of the PIN tool that demonstrates some basic PIN APIs 
 *  and could serve as the starting point for developing your first PIN tool
 */

#include "pin.H"
#include <iostream>
#include <fstream>
#include <sys/syscall.h>
#include <map>

using namespace std;
using std::cerr;
using std::string;
using std::endl;
using std::map;
/* ================================================================== */
// Global variables 
/* ================================================================== */

map<THREADID, BOOL> syscall_encountered_map;
map<THREADID, ADDRINT> syscall_num_map;

BOOL syscall_encountered = false; // if previous instruction was system call
ADDRINT syscall_num = -1;   // system call number of last system call executed

FILE * trace;

bool create_map (map<ADDRINT, string> &m) {

m[SYS_exit] = string("exit");
m[SYS_fork] = string("fork");
m[SYS_read] = string("read");
m[SYS_write] = string("write");
m[SYS_stat] = string("stat");
m[SYS_getpid] = string("getpid");
m[SYS_setuid] = string("setuid");
m[SYS_open] = string("open");
m[SYS_close] = string("close");
m[SYS_creat] = string("creat");
m[SYS_execve] = string("execve");
m[SYS_chdir] = string("chdir");
m[SYS_time] = string("time");
m[SYS_chmod] = string("chmod");
m[SYS_getuid] = string("getuid");
m[SYS_access] = string("access");
m[SYS_kill] = string("kill");
m[SYS_rename] = string("rename");
m[SYS_mkdir] = string("mkdir");
m[SYS_rmdir] = string("rmdir");
m[SYS_brk] = string("brk");
m[SYS_chroot] = string("chroot");
m[SYS_reboot] = string("reboot");
m[SYS_mprotect] = string("mprotect");
m[SYS_chown] = string("chown");
m[SYS_mmap2] = string("mmap2");
m[SYS_exit_group] = string("exit_group");
m[SYS_fstat] = string("fstat");
m[SYS_fstat64] = string("fstat64");
m[SYS_getdents] = string("getdents");
m[SYS_getdents64] = string("getdents64");
m[SYS_ioctl] = string("ioctl");
return true;
}

static map<ADDRINT, string> syscall_map;
static bool _dummy = create_map(syscall_map);

// std::ostream * out = &cerr;

/* ===================================================================== */
// Command line switches
/* ===================================================================== */
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE,  "pintool",
    "o", "", "specify file name for BtraceTool output");

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

VOID SysBefore (ADDRINT ip, CONTEXT * ctx, THREADID threadid) {

  ADDRINT num  = (ADDRINT) PIN_GetContextReg(ctx, LEVEL_BASE::REG_EAX);
  ADDRINT arg0 = (ADDRINT) PIN_GetContextReg(ctx, LEVEL_BASE::REG_EBX);
  ADDRINT arg1 = (ADDRINT) PIN_GetContextReg(ctx, LEVEL_BASE::REG_ECX);
  ADDRINT arg2 = (ADDRINT) PIN_GetContextReg(ctx, LEVEL_BASE::REG_EDX);
  ADDRINT arg3 = (ADDRINT) PIN_GetContextReg(ctx, LEVEL_BASE::REG_ESI);
  ADDRINT arg4 = (ADDRINT) PIN_GetContextReg(ctx, LEVEL_BASE::REG_EDI);


// }


// VOID _SysBefore (ADDRINT ip, ADDRINT num, ADDRINT arg0, ADDRINT arg1, ADDRINT arg2, ADDRINT arg3, ADDRINT arg4, ADDRINT arg5, THREADID threadid) {

	if (num == SYS_mmap)
  {
        ADDRINT * mmapArgs = reinterpret_cast<ADDRINT *>(arg0);
        arg0 = mmapArgs[0];
        arg1 = mmapArgs[1];
        arg2 = mmapArgs[2];
        arg3 = mmapArgs[3];
        arg4 = mmapArgs[4];
        // arg5 = mmapArgs[5];
  }

	int handled = syscall_map.count(num);

	if (handled < 1)
	{
    fprintf(trace,"%ld(0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx)",
        (unsigned long)num,
        (unsigned long)arg0,
        (unsigned long)arg1,
        (unsigned long)arg2,
        (unsigned long)arg3,
        (unsigned long)arg4);
        // (unsigned long)arg5);

		syscall_encountered = true;
  	syscall_num = num;
		return;
	}

  const char * syscall_name = syscall_map[num].c_str();

  if (num == SYS_exit)
  {
		fprintf(trace,"%s(%ld) = ?\n",
        syscall_name,
        (long) arg0);       
  }

  else if (num == SYS_fork)
  {
      fprintf(trace, "%s()",
       syscall_name);
  }

  else if (num == SYS_read || num == SYS_write)
  {
      char * temp_str = (char *)arg1;
      int temp_len = strlen(temp_str);

      char * temp_fmt_str;

        if (temp_len > 8) {
          temp_fmt_str = (char *)"%s(%d, \"%.8s\"..., %u)";
        } else {
          temp_fmt_str = (char *)"%s(%d, \"%s\", %u)";
        }

        fprintf(trace, temp_fmt_str,
          syscall_name,
          (int)arg0,
          (char *)arg1,
          (unsigned int)arg2);
  }

  else if (num == SYS_open)
  {
        int flag = ((int)arg1) & 3;
        
        char * mode;

        if (flag == 0) 
        {
          mode = (char *) "O_RDONLY";
        } 
        else if (flag == 1)        
        {
          mode = (char *) "O_WRONLY";
        }
        else
        {
          mode = (char *) "O_RDWR";
        }

        fprintf(trace, "%s(\"%s\", %s, %o)",
          syscall_name,
          (char *)arg0,
          mode,
          (unsigned int)arg2);
  }

  else if (num == SYS_close)
  {
      fprintf(trace,"%s(%ld)",
        syscall_name,
        (unsigned long)arg0);
  }

  else if (num == SYS_creat) 
  {
        fprintf(trace, "%s(\"%s\", %o)",
          syscall_name,
          (char *)arg0,
          (unsigned int)arg1);
  }

  else if (num == SYS_execve) 
  {
        fprintf(trace, "%s(\"%s\", 0x%lx, 0x%lx)",
          syscall_name,
          (char *)arg0,
          (unsigned long)arg1,
          (unsigned long)arg2);  
  }

	else if (num == SYS_chdir) 
  {
        fprintf(trace, "%s(\"%s\")",
          syscall_name,
          (char *)arg0);
  }

	else if (num == SYS_time) 
  {
    fprintf(trace,"%s(%ld) = ?\n",
        syscall_name,
        (long) arg0);  
  }

	else if (num == SYS_chmod) 
  {
        fprintf(trace, "%s(\"%s\", %o)",
          syscall_name,
          (char *)arg0,
					(unsigned int)arg1);
  }

	else if (num == SYS_stat) 
  {
        fprintf(trace, "%s(\"%s\", 0x%lx)",
          syscall_name,
          (char *)arg0,
          (unsigned long)arg1);  
  }

	else if (num == SYS_getpid) 
  {
      fprintf(trace, "%s()",
       syscall_name);  
  }

	else if (num == SYS_setuid) 
  {
      fprintf(trace,"%s(%lu)",
        syscall_name,
        (unsigned long)arg0); 
  }

	else if (num == SYS_getuid) 
  {
      fprintf(trace, "%s()",
       syscall_name);  
  }

	else if (num == SYS_access) 
  {
        fprintf(trace, "%s(\"%s\", %d)",
          syscall_name,
          (char *)arg0,
          (int)arg1);  
  }

	else if (num == SYS_kill) 
  {
        fprintf(trace, "%s(\"%ld\", %d)",
          syscall_name,
          (long int)arg0,
          (int)arg1);  
  }

	else if (num == SYS_rename) 
  {
        fprintf(trace, "%s(\"%s\", \"%s\")",
          syscall_name,
          (char *)arg0,
          (char *)arg1);  
  }

	else if (num == SYS_mkdir) 
  {
        fprintf(trace, "%s(\"%s\", %o)",
          syscall_name,
          (char *)arg0,
          (unsigned int)arg1);  
  }

	else if (num == SYS_rmdir) 
  {
        fprintf(trace, "%s(\"%s\")",
          syscall_name,
          (char *)arg0);  
  }

	else if (num == SYS_brk) 
  {
        fprintf(trace, "%s(0x%lx)",
          syscall_name,
          (unsigned long)arg0);  
  }

	else if (num == SYS_chroot)
  {
        fprintf(trace, "%s(\"%s\")",
          syscall_name,
          (char *)arg0);
  }

  else if (num == SYS_reboot)
  {
        fprintf(trace, "%s(\"%d\")",
          syscall_name,
          (int)arg0);
  }

  else if (num == SYS_mprotect)
  {
    fprintf(trace,"%s(0x%lx, %lu, %lu)",
        syscall_name,
        (unsigned long)arg0,
        (unsigned long)arg1,
        (unsigned long)arg2);
  }

  else if (num == SYS_chown)
  {
        fprintf(trace, "%s(\"%s\", %lu, %lu)",
          syscall_name,
          (char *)arg0,
          (unsigned long)arg1,
          (unsigned long)arg2);
  }

  else if (num == SYS_mmap2)
  {
        fprintf(trace, "%s(0x%lx, %u, %d, %d, %d)",
          syscall_name,
          (unsigned long)arg0,
          (unsigned int)arg1,
          (int)arg2,
          (int)arg3,
          (int)arg4//,
          // (unsigned long)arg5
          );

  }

  else if (num == SYS_exit_group)
  {
    fprintf(trace,"%s(%ld) = ?\n",
        syscall_name,
        (long) arg0);
  }

  else if (num == SYS_fstat || num == SYS_fstat64)
  {
    fprintf(trace, "%s(%d, 0x%lx)",
      syscall_name,
      (int)arg0,
      (unsigned long)arg1);
  }

  else if (num == SYS_getdents || num == SYS_getdents64)
  {
    fprintf(trace, "%s(%lu, 0x%lx, %lu)",
      syscall_name,
      (unsigned long)arg0,
      (unsigned long)arg1,
      (unsigned long)arg2);
  }

  else if (num == SYS_ioctl)
  {
    fprintf(trace, "%s(%d, %lu, 0x%lx)",
      syscall_name,
      (int)arg0,
      (unsigned long)arg1,
      (unsigned long)arg2);
  }

  else {
		fprintf(trace,"%ld(0x%lx, 0x%lx, 0x%lx, 0x%lx, 0x%lx)",
        (unsigned long)num,
        (unsigned long)arg0,
        (unsigned long)arg1,
        (unsigned long)arg2,
        (unsigned long)arg3,
        (unsigned long)arg4);
        //(unsigned long)arg5);
	}

  syscall_encountered_map[threadid] = true;
  syscall_num_map[threadid] = num;

}

VOID SysAfter(CONTEXT * ctx, THREADID threadid) {

  ADDRINT eax = (ADDRINT) PIN_GetContextReg(ctx, LEVEL_BASE::REG_EAX);

  if (!syscall_encountered_map[threadid]) { return; }

  ADDRINT num = syscall_num_map[threadid];
	syscall_encountered_map[threadid] = false;

	if (num == SYS_exit)
  {
		return;
  }

  else if (num == SYS_fork)
  {
		fprintf(trace," = %u\n", (unsigned int)eax);
  }

  else if (num == SYS_read || num == SYS_write)
  {
    if ((int)eax < 0) 
    {
    fprintf(trace," = -1 ERROR\n");
    }
    else 
    {
		fprintf(trace," = %lu\n", (unsigned long)eax);
    }
  }

  else if (num == SYS_open)
  {
		fprintf(trace," = %d\n", (int)eax);
  }

  else if (num == SYS_close)
  {
		fprintf(trace," = %d\n", (int)eax);
  }

  else if (num == SYS_creat)
  {
		fprintf(trace," = %d\n", (int)eax);
  }

  else if (num == SYS_execve)
  {
		fprintf(trace," = %d\n", (int)eax);
  }

  else if (num == SYS_chdir)
  {
		fprintf(trace," = %d\n", (int)eax);
  }

  else if (num == SYS_time)
  {
		fprintf(trace," = %lu\n", (unsigned long)eax);
  }

  else if (num == SYS_chmod)
  {
		fprintf(trace," = %d\n", (int)eax);
  }

  else if (num == SYS_stat)
  {
		fprintf(trace," = %d\n", (int)eax);
  }

  else if (num == SYS_getpid)
  {
		fprintf(trace," = %u\n", (unsigned int)eax);
  }

  else if (num == SYS_setuid)
  {
		fprintf(trace," = %d\n", (int)eax);
  }

  else if (num == SYS_getuid)
  {
		fprintf(trace," = %lu\n", (unsigned long)eax);
  }

  else if (num == SYS_access)
  {
    if ((int)eax != 0) 
    {
      fprintf(trace," = -1 UNSUCCESSFUL ACCESS\n");  
    }
    else
    {
		  fprintf(trace," = 0 SUCCESS ACCESS\n");
    }
  }

  else if (num == SYS_kill)
  {
		fprintf(trace," = %d\n", (int)eax);
  }

  else if (num == SYS_rename)
  {
		fprintf(trace," = %d\n", (int)eax);
  }

  else if (num == SYS_mkdir)
  {
		fprintf(trace," = %d\n", (int)eax);
  }

  else if (num == SYS_rmdir)
  {
		fprintf(trace," = %d\n", (int)eax);
  }

  else if (num == SYS_brk)
  {
		fprintf(trace," = %d\n", (int)eax);
  }

  else if (num == SYS_chroot)
  {
		fprintf(trace," = %d\n", (int)eax);
  }

  else if (num == SYS_reboot)
  {
		fprintf(trace," = %d\n", (int)eax);
  }

  else if (num == SYS_mprotect)
  {
		fprintf(trace," = %d\n", (int)eax);
  }

  else if (num == SYS_chown)
  {
		fprintf(trace," = %d\n", (int)eax);
  }

  else if (num == SYS_mmap2)
  {
		fprintf(trace," = 0x%lx\n", (unsigned long)eax);	
  }

  else if (num == SYS_exit_group)
  {
		return;
  }

  else if (num == SYS_fstat || num == SYS_fstat64)
  {
    fprintf(trace," = %d\n", (int)eax);
  }

  else if (num == SYS_ioctl)
  {
    fprintf(trace," = %d\n", (int)eax);
  }

	else
	{
		fprintf(trace," = 0x%lx\n", (unsigned long)eax);
	}

	fflush(trace);
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
VOID Trace(TRACE trace, VOID *v)
{
    // Visit every basic block in the trace
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        // Insert a call to CountBbl() before every basic bloc, passing the number of instructions
        BBL_InsertCall(bbl, IPOINT_BEFORE, (AFUNPTR)SysAfter,  IARG_CONTEXT, IARG_THREAD_ID, IARG_END);

        INS ins = BBL_InsTail(bbl);
        

        if ( INS_IsSyscall(ins) ) {
          // INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)CountSyscall, IARG_END);
          
          /*
					// Arguments and syscall number is only available before
        	INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SysBefore),
                       IARG_INST_PTR, IARG_SYSCALL_NUMBER,
                       IARG_SYSARG_VALUE, 0, IARG_SYSARG_VALUE, 1,
                       IARG_SYSARG_VALUE, 2, IARG_SYSARG_VALUE, 3,
                       IARG_SYSARG_VALUE, 4, IARG_SYSARG_VALUE, 5,
                       IARG_THREAD_ID,
                       IARG_END);
          */
          INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(SysBefore),
                       IARG_INST_PTR, IARG_CONTEXT, IARG_THREAD_ID, IARG_END);
				}

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
		fprintf(trace,"#eof\n");
		fclose(trace);
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

    _dummy = true;
    string fileName = KnobOutputFile.Value();

    if (!fileName.empty()) { 
			// out = new std::ofstream(fileName.c_str());
			trace = fopen(fileName.c_str(), "w");
		}

    if (KnobCount)
    {
        // Register function to be called to instrument traces
        TRACE_AddInstrumentFunction(Trace, 0);
        
        // Register function to be called when the application exits
        PIN_AddFiniFunction(Fini, 0);
    }

/*
    cerr <<  "===============================================" << endl;
    cerr <<  "This application is instrumented by BtraceTool" << endl;
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
