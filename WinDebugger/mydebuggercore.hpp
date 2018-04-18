#pragma once

#include <windows.h>
#include <tchar.h>
#include <thread>

class MyDebuggerCore {
public:
	enum class DebugAction {
		RESUME,
		PAUSE,
		STOP
	};
	enum DebuggerError {
		CANNOT_CREATE_DEBUGGEE_PROCESS = 1,
		CANNOT_LAUNCH_DEBUG_CYCLE_THREAD
	};

private:
	static const BYTE BP_INSTRUCTION = 0xCC;
	PROCESS_INFORMATION piDebuggee;
	STARTUPINFO siDebuggee;
	TCHAR * debuggeePath;
	TCHAR * debuggeeCmdParams;
	LPVOID pStartAddress;

	std::thread * MainDebugCycleThread;

	void terminateDebuggee();

	//int SetBreakPoint(/***/);
	//int ClearBreakPoint(/***/);
	//int StopDebugging();
	//int PauseDebugging();
	//int ResumeDebugging();

	static int DebugCycle(MyDebuggerCore *debuggingCore);

public:
	int StartDebugging(TCHAR * debuggeePath, TCHAR * cmdLineParams);
	int DebuggingAction(DebugAction && action);
	int DebuggingAction(DebugAction & action);
	std::thread * getThread() {
		return MainDebugCycleThread;
	}

protected:
	virtual DWORD ExceptionDebugEventHandler(EXCEPTION_DEBUG_INFO & stInfo, DWORD dwProcessId, DWORD dwThreadId) = 0;
	virtual DWORD CreateThreadDebugEventHandler(CREATE_THREAD_DEBUG_INFO & stInfo, DWORD dwProcessId, DWORD dwThreadId) = 0;
	virtual DWORD CreateProcessDebugEventHandler(CREATE_PROCESS_DEBUG_INFO & stInfo, DWORD dwProcessId, DWORD dwThreadId) = 0;
	virtual DWORD ExitThreadDebugEventHandler(EXIT_THREAD_DEBUG_INFO & stInfo, DWORD dwProcessId, DWORD dwThreadId) = 0;
	virtual DWORD ExitProcessDebugEventHandler(EXIT_PROCESS_DEBUG_INFO & stInfo, DWORD dwProcessId, DWORD dwThreadId) = 0;
	virtual DWORD LoadDllDebugEventHandler(LOAD_DLL_DEBUG_INFO & stInfo, DWORD dwProcessId, DWORD dwThreadId) = 0;
	virtual DWORD UnloadDllDebugEventHandler(UNLOAD_DLL_DEBUG_INFO & stInfo, DWORD dwProcessId, DWORD dwThreadId) = 0;
	virtual DWORD OutputDebugStringEventHandler(OUTPUT_DEBUG_STRING_INFO & stInfo, DWORD dwProcessId, DWORD dwThreadId) = 0;
	virtual DWORD RipEventHandler(RIP_INFO & stInfo, DWORD dwProcessId, DWORD dwThreadId) = 0;

	virtual DWORD DebuggerErrorHandler(DebuggerError debuggerError) = 0;
};
