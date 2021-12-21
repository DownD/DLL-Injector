#pragma once
#include <windows.h>
#include <iostream>
#include <stdio.h> 
#include <string>
#include <set>
#define LOADLIBRARY_A 1
#define MANUAL_MAP 2




class CDllMap {
public:
	virtual bool mapImage(HANDLE h,std::string dll) = 0;
};


class CManualMap :public CDllMap{
public:
	CManualMap(bool hijackThread);
	bool mapImage(HANDLE h, std::string dll);
private:
	bool hijackThread;

};

class CLoadLibrary :public CDllMap {
public:
	CLoadLibrary(bool hijackThread);
	bool mapImage(HANDLE h, std::string dll);
private:
	bool hijackThread;

};

class CInjector
{

public:
	CInjector();
	~CInjector();

	bool AutoInjectFromWindow(CDllMap& mapper, std::string dll, std::string processName);
	bool AutoInjectProcess(CDllMap& mapper, bool stealHandleJob, std::string dll, std::string processName);
	bool StartProcessAndInject(CDllMap & mapper, bool stealHandleJob, std::string file, std::string dll);
	static bool InjectFromPID(CDllMap& mapper, int pid, std::string dll);
	bool InjectFromClickingWindow(CDllMap& mapper, std::string dll);
private:


	struct ASAPArgs{
		HANDLE io_port;
		HANDLE returnHandle;
		std::string moduleName;
	};

	struct ASAPInjectionInfo {
		HANDLE io_port;
		std::string moduleName;
		CDllMap* map;
		std::string dllPath;
	};

	struct InjectNewProcessInfo {
		CDllMap* map;
		std::string dllPath;
		std::string processName;
	};


	//Steals a process handle
	HANDLE CreateProcessAndStealHandle(std::string& file);

	static void __InjectNewProcess(InjectNewProcessInfo* args);

	static BOOL CALLBACK __windowCallback(HWND hWnd, LPARAM wParam);

	static void __handleRecieverASAP(ASAPArgs* args);
	static void __handleRecieverASAPNoReturn(ASAPInjectionInfo* args);

private:
	//Used by process scanner
	static std::set<int> m_lastProcesses;

	//Used by window scanner stupid callbacks
	static std::set<int> m_injectedProcesses; //Avoid injecting the same process
	static bool isFirstTime;
	static std::set<DWORD> m_lastWnds; //last windows
	static std::set<DWORD> m_newWnds;	//Current windows
};

