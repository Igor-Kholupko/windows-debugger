#pragma once

#include <windows.h>
#include <tchar.h>



class MyDebuggerCore {
	enum DebugAction {
		RESUME,
		PAUSE,
		STOP
	};

private:
	HANDLE hMainDebugCycleThread;

	HANDLE hDebuggeeProcess;
	PROCESS_INFORMATION piDebuggee;

	int SetBreakPoint(/***/);
	int ClearBreakPoint(/***/);
	int StopDebugging();
	int PauseDebugging();
	int ResumeDebugging();

	int DebugCycle();

public:
	int StartDebugging(TCHAR debuggeePath, TCHAR cmdLineParams);
	int DebuggingAction(DebugAction action);

protected:
	virtual DWORD ExceptionDebugEventHandler(EXCEPTION_DEBUG_INFO& stInfo, DWORD& dwProcessId, DWORD& dwThreadId) = 0;
	virtual DWORD CreateThreadDebugEventHandler(CREATE_THREAD_DEBUG_INFO& stInfo, DWORD& dwProcessId, DWORD& dwThreadId) = 0;
	virtual DWORD CreateProcessDebugEventHandler(CREATE_PROCESS_DEBUG_INFO& stInfo, DWORD& dwProcessId, DWORD& dwThreadId) = 0;
	virtual DWORD ExitThreadDebugEventHandler(EXIT_THREAD_DEBUG_INFO& stInfo, DWORD& dwProcessId, DWORD& dwThreadId) = 0;
	virtual DWORD ExitProcessDebugEventHandler(EXIT_PROCESS_DEBUG_INFO& stInfo, DWORD& dwProcessId, DWORD& dwThreadId) = 0;
	virtual DWORD LoadDllDebugEventHandler(LOAD_DLL_DEBUG_INFO& stInfo, DWORD& dwProcessId, DWORD& dwThreadId) = 0;
	virtual DWORD UnloadDllDebugEventHandler(UNLOAD_DLL_DEBUG_INFO& stInfo, DWORD& dwProcessId, DWORD& dwThreadId) = 0;
	virtual DWORD OutputDebugStringEventHandler(OUTPUT_DEBUG_STRING_INFO& stInfo, DWORD& dwProcessId, DWORD& dwThreadId) = 0;
	virtual DWORD RipEventHandler(RIP_INFO& stInfo, DWORD& dwProcessId, DWORD& dwThreadId) = 0;

};
