#pragma once

#include <Windows.h>
#include <DbgHelp.h>
#include <vector>

class SymHandler
{
public:
	enum class SYM_HANDLER_STATUS : int {
		ALL_RIGHT,
		SYM_INIT_ERROR,
		SYM_LOAD_MODULE_ERROR,
		SYM_MODULE_INFO_ERROR
	};
private:
	SYM_HANDLER_STATUS _status = SYM_HANDLER_STATUS::ALL_RIGHT;
	BOOL bMainModule;

	HANDLE hProcess;
	DWORD64 BaseOfDll;
	DWORD64 dwBase;
	IMAGEHLP_MODULE64 module_info;
	std::vector<std::pair<	SOURCEFILE,
							std::vector<SRCCODEINFO>
				>> sourceFiles;
public:
	SymHandler(HANDLE hProcess,
			   HANDLE hFile,
			   PCSTR ImageName,
			   PCSTR ModuleName,
			   DWORD64 BaseOfDll,
			   DWORD SizeOfDll,
			   BOOL bMainModule = FALSE);
	~SymHandler();

	SYM_HANDLER_STATUS status() {
		return _status;
	}
};
