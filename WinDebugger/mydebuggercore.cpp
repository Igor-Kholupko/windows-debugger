#include "mydebuggercore.hpp"

void MyDebuggerCore::terminateDebuggee() {
	TerminateProcess(piDebuggee.hProcess, 1);
	CloseHandle(piDebuggee.hProcess);
	CloseHandle(piDebuggee.hThread);
}

int MyDebuggerCore::DebugCycle(MyDebuggerCore * debuggingCore) {
	DEBUG_EVENT stDE;
	BOOL bSeenInitBP = FALSE;
	BOOL bSeenInitAddress = FALSE;
	BOOL bRet = FALSE;
	DWORD dwContinueStatus;

	bRet = CreateProcessW(debuggingCore->debuggeePath,
						  debuggingCore->debuggeeCmdParams,
						  NULL,
						  NULL,
						  FALSE,
						  CREATE_NEW_CONSOLE | DEBUG_ONLY_THIS_PROCESS,
						  NULL,
						  NULL,
						  &debuggingCore->siDebuggee,
						  &debuggingCore->piDebuggee);
	if(bRet == FALSE) {
		debuggingCore->DebuggerErrorHandler(DebuggerError::CANNOT_CREATE_DEBUGGEE_PROCESS);
		return (int) (DebuggerError::CANNOT_CREATE_DEBUGGEE_PROCESS);
	}

	while(WaitForSingleObject(debuggingCore->piDebuggee.hProcess, 0) != WAIT_OBJECT_0) {
		bRet = WaitForDebugEvent(&stDE, 100);

		if(bRet == TRUE) {
			switch(stDE.dwDebugEventCode) {

			case EXCEPTION_DEBUG_EVENT:
				switch(stDE.u.Exception.ExceptionRecord.ExceptionCode) {
				case EXCEPTION_BREAKPOINT:
					if(bSeenInitBP == FALSE) {
						bSeenInitBP = TRUE;
						dwContinueStatus = DBG_CONTINUE;
					}
					else {
						dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
					}
				default:
					dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
				}
				debuggingCore->ExceptionDebugEventHandler(stDE.u.Exception, stDE.dwProcessId, stDE.dwThreadId);
				break;

			case CREATE_THREAD_DEBUG_EVENT:
				debuggingCore->CreateThreadDebugEventHandler(stDE.u.CreateThread, stDE.dwProcessId, stDE.dwThreadId);
				dwContinueStatus = DBG_CONTINUE;
				break;

			case CREATE_PROCESS_DEBUG_EVENT:
				if(bSeenInitAddress == FALSE) {
					debuggingCore->pStartAddress = (LPVOID) stDE.u.CreateProcessInfo.lpStartAddress;
					bSeenInitAddress = TRUE;
				}
				debuggingCore->CreateProcessDebugEventHandler(stDE.u.CreateProcessInfo, stDE.dwProcessId, stDE.dwThreadId);
				dwContinueStatus = DBG_CONTINUE;
				break;

			case EXIT_THREAD_DEBUG_EVENT:
				debuggingCore->ExitThreadDebugEventHandler(stDE.u.ExitThread, stDE.dwProcessId, stDE.dwThreadId);
				dwContinueStatus = DBG_CONTINUE;
				break;

			case EXIT_PROCESS_DEBUG_EVENT:
				debuggingCore->ExitProcessDebugEventHandler(stDE.u.ExitProcess, stDE.dwProcessId, stDE.dwThreadId);
				dwContinueStatus = DBG_CONTINUE;
				break;

			case LOAD_DLL_DEBUG_EVENT:
				debuggingCore->LoadDllDebugEventHandler(stDE.u.LoadDll, stDE.dwProcessId, stDE.dwThreadId);
				dwContinueStatus = DBG_CONTINUE;
				break;

			case UNLOAD_DLL_DEBUG_EVENT:
				debuggingCore->UnloadDllDebugEventHandler(stDE.u.UnloadDll, stDE.dwProcessId, stDE.dwThreadId);
				dwContinueStatus = DBG_CONTINUE;
				break;

			case OUTPUT_DEBUG_STRING_EVENT:
				debuggingCore->OutputDebugStringEventHandler(stDE.u.DebugString, stDE.dwProcessId, stDE.dwThreadId);
				dwContinueStatus = DBG_CONTINUE;
				break;

			case RIP_EVENT:
				debuggingCore->RipEventHandler(stDE.u.RipInfo, stDE.dwProcessId, stDE.dwThreadId);
				dwContinueStatus = DBG_CONTINUE;
				break;
			}
			ContinueDebugEvent(stDE.dwProcessId, stDE.dwThreadId, dwContinueStatus);
		}
	}
	return 0;
}

int MyDebuggerCore::StartDebugging(TCHAR * debuggeePath, TCHAR * debuggeeCmdParams) {

	this->debuggeePath = debuggeePath;
	this->debuggeeCmdParams = debuggeeCmdParams;

	memset(&piDebuggee, NULL, sizeof(PROCESS_INFORMATION));
	memset(&siDebuggee, NULL, sizeof(STARTUPINFO));

	siDebuggee.cb = sizeof(STARTUPINFO);

	try {
		MainDebugCycleThread = new std::thread(&MyDebuggerCore::DebugCycle, this);
	}
	catch(std::system_error) {
		DebuggerErrorHandler(DebuggerError::CANNOT_LAUNCH_DEBUG_CYCLE_THREAD);
	}
	return 0;
}
