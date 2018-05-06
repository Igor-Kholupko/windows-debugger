#include "mydebuggercore.hpp"
#include "getfilenamefromhandle.hpp"
#include <iostream>
#include <DbgHelp.h>

void MyDebuggerCore::terminateDebuggee() {
	TerminateProcess(piDebuggee.hProcess, 1);
	CloseHandle(piDebuggee.hProcess);
	CloseHandle(piDebuggee.hThread);
}

int MyDebuggerCore::DebugCycle(MyDebuggerCore * debuggingCore) {
	DEBUG_EVENT stDE;
	BOOL bSeenInitBP = FALSE;
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
		debuggingCore->DebuggerErrorHandler(DebuggerError::ERR_CANNOT_CREATE_DEBUGGEE_PROCESS);
		return (int) (DebuggerError::ERR_CANNOT_CREATE_DEBUGGEE_PROCESS);
	}

	while(WaitForSingleObject(debuggingCore->piDebuggee.hProcess, 0) != WAIT_OBJECT_0) {
		bRet = WaitForDebugEvent(&stDE, 100);

		if(bRet == TRUE) {
			switch(stDE.dwDebugEventCode) {

			case EXCEPTION_DEBUG_EVENT: {
				switch(stDE.u.Exception.ExceptionRecord.ExceptionCode) {
				case EXCEPTION_BREAKPOINT:
					if(bSeenInitBP == FALSE) {
						bSeenInitBP = TRUE;
						dwContinueStatus = DBG_CONTINUE;
					}
					else {
						debuggingCore->BreakPointHandler(stDE);
						dwContinueStatus = DBG_CONTINUE;
					}
					break;
				default:
					dwContinueStatus = DBG_EXCEPTION_NOT_HANDLED;
				}
				std::lock_guard<std::mutex>(debuggingCore->dcMutex);
				debuggingCore->ExceptionDebugEventHandler(stDE.u.Exception, stDE.dwProcessId, stDE.dwThreadId);
				break;
			}

			case CREATE_THREAD_DEBUG_EVENT: {
				std::lock_guard<std::mutex>(debuggingCore->dcMutex);
				debuggingCore->CreateThreadDebugEventHandler(stDE.u.CreateThread, stDE.dwProcessId, stDE.dwThreadId);
				debuggingCore->debugeeThreads.push_back(std::make_pair(stDE.u.CreateThread.hThread, stDE.dwThreadId));
				dwContinueStatus = DBG_CONTINUE;
				break;
			}

			case CREATE_PROCESS_DEBUG_EVENT: {
				std::lock_guard<std::mutex>(debuggingCore->dcMutex);

				bRet = SymInitialize(stDE.u.CreateProcessInfo.hProcess, NULL, FALSE);
				if(bRet == FALSE) {
					std::cout << "SymInitialize error : " << std::hex << GetLastError() << std::endl;
				}
				else {
					DWORD64 dwBase = SymLoadModule64(stDE.u.CreateProcessInfo.hProcess, stDE.u.CreateProcessInfo.hFile,(PCSTR) stDE.u.CreateProcessInfo.lpImageName, NULL, (DWORD64) stDE.u.CreateProcessInfo.lpBaseOfImage, stDE.u.CreateProcessInfo.nDebugInfoSize);
					if(dwBase == FALSE) {
						std::cout << "SymLoadModule error : " << std::hex << GetLastError() << std::endl;
					}
					else {
						std::cout << std::hex << dwBase << std::endl;
					}
				}

				CloseHandle(stDE.u.CreateProcessInfo.hFile);
				debuggingCore->CreateProcessDebugEventHandler(stDE.u.CreateProcessInfo, stDE.dwProcessId, stDE.dwThreadId);
				dwContinueStatus = DBG_CONTINUE;
				break;
			}

			case EXIT_THREAD_DEBUG_EVENT: {
				std::lock_guard<std::mutex>(debuggingCore->dcMutex);
				debuggingCore->ExitThreadDebugEventHandler(stDE.u.ExitThread, stDE.dwProcessId, stDE.dwThreadId);
				int i = 0;
				for(std::pair<HANDLE, DWORD> p : debuggingCore->debugeeThreads) {
					if(p.second == stDE.dwThreadId) {
						debuggingCore->debugeeThreads.erase(debuggingCore->debugeeThreads.begin() + i);
						break;
					}
					++i;
				}
				dwContinueStatus = DBG_CONTINUE;
				break;
			}

			case EXIT_PROCESS_DEBUG_EVENT: {
				std::lock_guard<std::mutex>(debuggingCore->dcMutex);
				debuggingCore->ExitProcessDebugEventHandler(stDE.u.ExitProcess, stDE.dwProcessId, stDE.dwThreadId);
				dwContinueStatus = DBG_CONTINUE;
				break;
			}

			case LOAD_DLL_DEBUG_EVENT: {
				std::lock_guard<std::mutex>(debuggingCore->dcMutex);
				HANDLE sddf = debuggingCore->piDebuggee.hProcess;
				TCHAR * aaa;
				GetFileNameFromHandle(stDE.u.LoadDll.hFile, aaa);
				DWORD64 dwBase = SymLoadModule64(sddf, NULL, (PCSTR) aaa,
					 0, (DWORD64)stDE.u.LoadDll.lpBaseOfDll, stDE.u.LoadDll.nDebugInfoSize);
				std::cout << std::hex << dwBase << ' ';
				debuggingCore->LoadDllDebugEventHandler(stDE.u.LoadDll, stDE.dwProcessId, stDE.dwThreadId);
				dwContinueStatus = DBG_CONTINUE;
				break;
			}

			case UNLOAD_DLL_DEBUG_EVENT: {
				std::lock_guard<std::mutex>(debuggingCore->dcMutex);
				debuggingCore->UnloadDllDebugEventHandler(stDE.u.UnloadDll, stDE.dwProcessId, stDE.dwThreadId);
				dwContinueStatus = DBG_CONTINUE;
				break;
			}

			case OUTPUT_DEBUG_STRING_EVENT: {
				std::lock_guard<std::mutex>(debuggingCore->dcMutex);
				debuggingCore->OutputDebugStringEventHandler(stDE.u.DebugString, stDE.dwProcessId, stDE.dwThreadId);
				dwContinueStatus = DBG_CONTINUE;
				break;
			}

			case RIP_EVENT: {
				std::lock_guard<std::mutex>(debuggingCore->dcMutex);
				debuggingCore->RipEventHandler(stDE.u.RipInfo, stDE.dwProcessId, stDE.dwThreadId);
				dwContinueStatus = DBG_CONTINUE;
				break;
			}
			}
			ContinueDebugEvent(stDE.dwProcessId, stDE.dwThreadId, dwContinueStatus);
		}
	}
	CloseHandle(debuggingCore->hBPEvent);
	return 0;
}

int MyDebuggerCore::SetBreakPoint(DWORD dwBPAddress) {
	BYTE cInstruction;
	DWORD dwReadBytes;

	ReadProcessMemory(piDebuggee.hProcess, (LPVOID) dwBPAddress, &cInstruction, 1, &dwReadBytes);

	if(dwReadBytes == 0)
		return (int) DebuggerError::ERR_CANNOT_READ_PROCESS_MEMORY;

	breakPoints.push_back(std::make_pair(dwBPAddress, cInstruction));

	cInstruction = BP_INSTRUCTION;
	WriteProcessMemory(piDebuggee.hProcess, (LPVOID) dwBPAddress, &cInstruction, 1, &dwReadBytes);

	if(dwReadBytes == 0) {
		breakPoints.pop_back();
		return (int) DebuggerError::ERR_CANNOT_SET_BREAK_POINT;
	}

	FlushInstructionCache(piDebuggee.hProcess, (LPVOID) dwBPAddress, 1);
	return 0;
}
int MyDebuggerCore::ClearBreakPoint(DWORD dwBPAddress) {
	for(std::pair<DWORD, BYTE> p : breakPoints) {
		if(p.first == dwBPAddress) {
			WriteProcessMemory(piDebuggee.hProcess, (LPVOID) dwBPAddress, &p.second, 1, NULL);
			FlushInstructionCache(piDebuggee.hProcess, (LPVOID) dwBPAddress, 1);
			return 0;
		}
	}
	return (int) DebuggerError::ERR_CANNOT_FIND_BREAK_POINT;
}

void MyDebuggerCore::BreakPointHandler(DEBUG_EVENT & stDE) {
	ExceptionBreakPointDebugEventHandler(stDE.u.Exception, stDE.dwProcessId, stDE.dwThreadId);
	WaitForSingleObject(hBPEvent, INFINITE);
	if(bDebugPaused == FALSE)
		ClearCurrentBreakPoint((DWORD)stDE.u.Exception.ExceptionRecord.ExceptionAddress, stDE.dwThreadId);
	bDebugPaused = FALSE;
}

void MyDebuggerCore::ClearCurrentBreakPoint(DWORD dwBPAddress, DWORD dwThreadId) {
	if(ClearBreakPoint(dwBPAddress) == 0) {
		HANDLE hThread;
		for(std::pair<HANDLE, DWORD> p : debugeeThreads) {
			if(p.second == dwThreadId) {
				hThread = p.first;
				break;
			}
		}
		CONTEXT lcContext;
		lcContext.ContextFlags = CONTEXT_ALL;
		GetThreadContext(hThread, &lcContext);
		--lcContext.Eip;
		SetThreadContext(hThread, &lcContext);
	}
}

int MyDebuggerCore::PauseDebugging() {
	if(DebugBreakProcess(piDebuggee.hProcess) == FALSE) {
		DebuggerErrorHandler(DebuggerError::ERR_CANNOT_PAUSE_DEBUGGEE_PROCESS);
		return (int) DebuggerError::ERR_CANNOT_PAUSE_DEBUGGEE_PROCESS;
	}

	bDebugPaused = TRUE;
	return 0;
}

int MyDebuggerCore::ResumeDebugging() {
	if(WaitForSingleObject(hBPEvent, 0) != WAIT_OBJECT_0) {
		if(SetEvent(hBPEvent) == TRUE)
			return 0;
		return (int) DebuggerError::ERR_CANNOT_RESUME_DEBUGEE_PROCESS;
	}
	return (int) DebuggerError::ERR_CANNOT_FIND_BREAK_POINT;
}

void MyDebuggerCore::StopDebugging() {
	ResumeDebugging();
	terminateDebuggee();
}

int MyDebuggerCore::StartDebugging(TCHAR * debuggeePath, TCHAR * debuggeeCmdParams) {

	this->debuggeePath = debuggeePath;
	this->debuggeeCmdParams = debuggeeCmdParams;

	memset(&piDebuggee, NULL, sizeof(PROCESS_INFORMATION));
	memset(&siDebuggee, NULL, sizeof(STARTUPINFO));

	siDebuggee.cb = sizeof(STARTUPINFO);

	bDebugPaused = FALSE;

	hBPEvent = CreateEventW(NULL, FALSE, FALSE, NULL);

	try {
		MainDebugCycleThread = new std::thread(&MyDebuggerCore::DebugCycle, this);
	}
	catch(std::system_error) {
		DebuggerErrorHandler(DebuggerError::ERR_CANNOT_LAUNCH_DEBUG_CYCLE_THREAD);
		return (int) DebuggerError::ERR_CANNOT_LAUNCH_DEBUG_CYCLE_THREAD;
	}
	return 0;
}

std::thread * MyDebuggerCore::getThread() {
	return MainDebugCycleThread;
}
