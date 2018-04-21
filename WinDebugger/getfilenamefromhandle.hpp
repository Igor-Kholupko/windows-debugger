#pragma once

//TODO:
//Упаковать в DLL

typedef int BOOL;
typedef void* HANDLE;
typedef wchar_t WCHAR;
typedef WCHAR TCHAR;

BOOL GetFileNameFromHandle(HANDLE hFile, WCHAR *& output);
