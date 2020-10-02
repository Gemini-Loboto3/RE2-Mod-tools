#pragma once
#include <string>

class CFile
{
public:
	CFile() { handle = INVALID_HANDLE_VALUE; }
	CFile(LPCSTR name, bool bOpen = true)
	{
		handle = INVALID_HANDLE_VALUE;
		if(bOpen) Open(name);
		else Create(name);
	}
	~CFile() { Close(); }

	int Open(LPCSTR filename);
	int Create(LPCTSTR filename);
	void Close();

	DWORD GetSize();
	DWORD GetPosition() { return SetFilePointer(handle, 0, NULL, FILE_CURRENT); }
	void SetPosition(DWORD pos, DWORD move_method) { SetFilePointer(handle, pos, NULL, move_method); }
	void GetFinalPath();

	__inline const char* GetOpenPath() { return open_path.c_str(); }

	DWORD Read(LPVOID buffer, DWORD size);
	DWORD Write(LPVOID buffer, DWORD size);

	__inline bool IsOpen() { return handle != INVALID_HANDLE_VALUE; }

private:
	HANDLE handle;
	std::string open_path;
};
