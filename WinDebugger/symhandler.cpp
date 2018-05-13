#include "symhandler.hpp"

#include <filesystem>
#include <iostream>

SymHandler::SymHandler(HANDLE hProcess, HANDLE hFile, PCSTR ImageName, PCSTR ModuleName, DWORD64 BaseOfDll, DWORD SizeOfDll, BOOL bMainModule) : hProcess(hProcess), BaseOfDll(BaseOfDll), bMainModule(bMainModule) {

	BOOL bSuccess;
	if(bMainModule == TRUE) {
		bSuccess = SymInitialize(hProcess, NULL, FALSE);
		if(bSuccess == FALSE) {
			int a = GetLastError();
			_status = SYM_HANDLER_STATUS::SYM_INIT_ERROR;
			return;
		}
	}

	dwBase = SymLoadModule64(hProcess, hFile, ImageName, ModuleName, BaseOfDll, SizeOfDll);
	if(dwBase == FALSE) {
		int a = GetLastError();
		if(bMainModule == TRUE)
			SymCleanup(hProcess);
		_status = SYM_HANDLER_STATUS::SYM_LOAD_MODULE_ERROR;
		return;
	}
	else {
		module_info.SizeOfStruct = sizeof(module_info);
		bSuccess = SymGetModuleInfo64(hProcess, dwBase, &module_info);

		// Check and notify
		if(bSuccess == TRUE && module_info.SymType == SymPdb)
		{
			std::cout << "Symbols Loaded." << std::endl;

			struct args_struct {
				std::vector<std::pair<SOURCEFILE, std::vector<SRCCODEINFO>>> * vector;
				HANDLE * hProcessArg;
				DWORD64 * dwBaseArg;
			} UserInfo;

			UserInfo.vector = &sourceFiles;
			UserInfo.hProcessArg = &hProcess;
			UserInfo.dwBaseArg = &dwBase;

			SymEnumSourceFiles(hProcess, dwBase,
							   "*.cpp", [](PSOURCEFILE pSourceFile, PVOID args_void) -> BOOL {
				std::vector<std::pair<SOURCEFILE, std::vector<SRCCODEINFO>>> * vector = reinterpret_cast<args_struct *>(args_void)->vector;
				HANDLE hProcess = *(reinterpret_cast<args_struct *>(args_void)->hProcessArg);
				DWORD64 dwBase = *(reinterpret_cast<args_struct *>(args_void)->dwBaseArg);
				if(std::experimental::filesystem::exists(std::experimental::filesystem::path(pSourceFile->FileName))) {
					std::pair<SOURCEFILE, std::vector<SRCCODEINFO>> temp;
					temp.first.ModBase = pSourceFile->ModBase;
					int len = strlen(pSourceFile->FileName);
					temp.first.FileName = new char[len+1]();
					memcpy(temp.first.FileName, pSourceFile->FileName, len);

					SymEnumLines(hProcess, dwBase, NULL, pSourceFile->FileName,
								 [](PSRCCODEINFO LineInfo, PVOID args_void) -> BOOL {
						std::vector<SRCCODEINFO> * vector = reinterpret_cast<std::vector<SRCCODEINFO> *>(args_void);
						SRCCODEINFO tmp;
						memcpy(&tmp, LineInfo, LineInfo->SizeOfStruct);
						vector->push_back(tmp);
						return TRUE;
					}, &temp.second);

					vector->push_back(temp);
					return TRUE;
				}
				return FALSE;
			}, &UserInfo);
		}
		else {
			SymUnloadModule64(hProcess, BaseOfDll);
			if(bMainModule == TRUE)
				SymCleanup(hProcess);
			_status = SYM_HANDLER_STATUS::SYM_MODULE_INFO_ERROR;
			return;
		}
	}
}

SymHandler::~SymHandler() {
	if(_status == SYM_HANDLER_STATUS::ALL_RIGHT) {
		for(int i = 0; i < sourceFiles.size(); ++i)
			delete [] sourceFiles.at(i).first.FileName;
		SymUnloadModule64(hProcess, BaseOfDll);
		if(bMainModule == TRUE)
			SymCleanup(hProcess);
	}
}
