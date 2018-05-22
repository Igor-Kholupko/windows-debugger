#include "getfilenamefromhandle.hpp"

#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <string.h>
#include <psapi.h>
#include <strsafe.h>

#define BUFSIZE 512

BOOL GetFileNameFromHandle(HANDLE hFile, TCHAR *& output) {
	BOOL bSuccess = FALSE;
	TCHAR pszFilename[MAX_PATH+1];
	output = new TCHAR[MAX_PATH+1];
	output[0] = 0;
	HANDLE hFileMap;

	//Получение размера файла.
	DWORD dwFileSizeHi = 0;
	DWORD dwFileSizeLo = GetFileSize(hFile, &dwFileSizeHi);

	if( dwFileSizeLo == 0 && dwFileSizeHi == 0 ) {
		return FALSE;
	}

	//Создание объекта файла отражаемой памяти на базе файла, имя которого нужно получить.
	hFileMap = CreateFileMapping(hFile,
								NULL,
								PAGE_READONLY,
								0,
								1,
								NULL);

	if (hFileMap)
	{
		//Получение памяти файла.
		void* pMem = MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 1);

		if (pMem)
		{
			//Получения имени файла (без буквы диска, но с именем устройства)
			if (GetMappedFileName(GetCurrentProcess(),
									pMem,
									pszFilename,
									MAX_PATH))
			{

				//Получение все букв логических дисков
				TCHAR szTemp[BUFSIZE];
				szTemp[0] = '\0';

				if (GetLogicalDriveStrings(BUFSIZE-1, szTemp))
				{
					TCHAR szName[MAX_PATH];
					TCHAR szDrive[3] = TEXT(" :");
					BOOL bFound = FALSE;
					TCHAR* p = szTemp;

					do
					{
						//Копирование буквы диска во временную строку
						*szDrive = *p;

						//Получение имени устройства из имени диска
						if (QueryDosDevice(szDrive, szName, MAX_PATH))
						{
							size_t uNameLen = _tcslen(szName);

							if (uNameLen < MAX_PATH)
							{
								//Проверка совпадения имени устройств
								bFound = _tcsnicmp(pszFilename, szName, uNameLen) == 0
								&& *(pszFilename + uNameLen) == _T('\\');

								if (bFound)
								{
									//Изменение имени файла, с установкой буквы диска
									TCHAR szTempFile[MAX_PATH];
									StringCchPrintf(szTempFile,
													MAX_PATH,
													TEXT("%s%s"),
													szDrive,
													pszFilename+uNameLen);
													StringCchCopyN(pszFilename, MAX_PATH+1, szTempFile, _tcslen(szTempFile));
									_tcscpy(output, pszFilename);
								}
							}
						}

						//Переход на следующую букву
						while (*p++);
					} while (!bFound && *p); //Конец строки или диск найден
				}
			}
			bSuccess = TRUE;
			UnmapViewOfFile(pMem);
		}
		//Закрываем файл
		CloseHandle(hFileMap);
	}
	return(bSuccess);
}
