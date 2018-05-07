#pragma once

#include <windows.h>
#include <tchar.h>
#include <thread>
#include <mutex>
#include <vector>

class MyDebuggerCore {
public:
	//Класс перечислений для выбора действия
	enum class DebugAction : int {
		//Продолжить отладку
		RESUME,
		//Приостановить отладку
		PAUSE,
		//Остановить (завершить) отладку
		STOP
	};
	//Класс перечислений ошибок ОТЛАДЧИКА
	enum class DebuggerError : int {
		NO_ERR,
		//Не удаётся запустить отлаживаемый процесс
		ERR_CANNOT_CREATE_DEBUGGEE_PROCESS,
		//Не удаётся поток основного отладочного цикла
		ERR_CANNOT_LAUNCH_DEBUG_CYCLE_THREAD,
		//Не удаётся приостановит отлаживаемый процесс
		ERR_CANNOT_PAUSE_DEBUGGEE_PROCESS,
		//Не удаётся продолжитьт отладку
		ERR_CANNOT_RESUME_DEBUGEE_PROCESS,
		//При попытке продолжить отладку, если отадка не остановлена
		ERR_DEBUGGEE_ALREADY_ACTIVE,
		//Не удаётся прочитать память процесса
		ERR_CANNOT_READ_PROCESS_MEMORY,
		//Не удаётся установить точку останова
		ERR_CANNOT_SET_BREAK_POINT,
		//Не удаётся найти такую точку останова
		ERR_CANNOT_FIND_BREAK_POINT
	};

public:
	//Машинный код точки останова (прерывание __asm int 3)
	static const BYTE BP_INSTRUCTION = 0xCC;

	//Структуры, хранящие информацию о отлаживаемом процессе
	PROCESS_INFORMATION piDebuggee;
	STARTUPINFO siDebuggee;
	//Путь и аргументы командной строки для отлаживаемого процесса
	TCHAR * debuggeePath;
	TCHAR * debuggeeCmdParams;
	//Флаг для отличия исключений, вызванных пользовательскими точками останова,
	//от исключений, вызванных методом PauseDebugging()
	BOOL bDebugPaused;
	//Останавливает отладочный цикл, когда попадает на точку останова
	HANDLE hBPEvent;

	std::vector<std::pair<
					DWORD/*Адрес точки*/,
					BYTE/*Оригинальная инструкция по этому адресу*/
				>> breakPoints;
	std::vector<std::pair<
					HANDLE/*Описатель потока*/,
					DWORD/*ID потока*/
				>> debugeeThreads;

	//Поток, выполняющий основной отладочный цикл
	std::thread * mainDebugCycleThread;
	//Мьютекс для синхронного доступа к интерфейсу
	std::mutex dcMutex;

	//Принудительное завершение отлаживаемого процесса
	void terminateDebuggee();

	//Установка точки останова
	//Параметры:
	//dwBPAddress - адрес точки останова
	int SetBreakPoint(DWORD dwBPAddress);
	//Удаление точки останова
	//Параметры:
	//dwBPAddress - адрес точки останова
	int ClearBreakPoint(DWORD dwBPAddress);
	//Обработка точки останова (локльная)
	//Параметры:
	//stDE - структура DEBUG_EVENT при возникновении DEBUG_EXCEPTION_EVENT с кодом EXCEPTION_BREAKPOINT
	void BreakPointHandler(DEBUG_EVENT & stDE);
	//Удалть текущую точку останова
	//Параметры:
	//dwBPAddress - адрес точки останова
	//dwThreadId - ID потока, в котором возникла остановка
	void ClearCurrentBreakPoint(DWORD dwBPAddress, DWORD dwThreadId);

	//Остановить отладку
	void StopDebugging();
	//Приостановить отладку
	int PauseDebugging();
	//Продолжить отладку
	int ResumeDebugging();
	//Шаг с заходом
	int SingleStep();


	//Основной отладочный цикл
	//Параметры:
	//debuggingCore - указатель на ядро отладчика
	static int DebugCycle(MyDebuggerCore * debuggingCore);

public:
	//Запуск отладчика
	//Параметры:
	//debuggeePath - путь к отлаживаемой программе
	//cmdLineParams - аргументы командной строки для отлаживаемого процесса
	int StartDebugging(TCHAR * debuggeePath, TCHAR * cmdLineParams);
	//Потльзовательское действие
	//Параметры:
	//action - выбор действия
	int DebuggingAction(DebugAction && action);
	int DebuggingAction(DebugAction & action);

	std::thread * getThread();

protected:
	//Виртуальные методы для вывода информации, определяемые в интерфейсе
	virtual DWORD ExceptionDebugEventHandler(EXCEPTION_DEBUG_INFO & stInfo, DWORD dwProcessId, DWORD dwThreadId) = 0;
	virtual DWORD ExceptionBreakPointDebugEventHandler(EXCEPTION_DEBUG_INFO & stInfo, DWORD dwProcessId, DWORD dwThreadId) = 0;
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
