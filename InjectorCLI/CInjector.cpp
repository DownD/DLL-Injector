#include "CInjector.h"
#include "BlackBone/Process/Process.h"
#include "BlackBone/PE/PEImage.h"
#include <locale>
#include <codecvt>
#include "util.hpp"
#include "psapi.h"


std::set<int> CInjector::m_lastProcesses;
std::set<int> CInjector::m_injectedProcesses; //Avoid injecting the same process
std::set<DWORD> CInjector::m_lastWnds; //last windows
std::set<DWORD> CInjector::m_newWnds;	//Current windows
bool CInjector::isFirstTime= true;

CInjector::CInjector()
{
}

CInjector::~CInjector()
{

}

bool CInjector::AutoInjectFromWindow(CDllMap& mapper, std::string dll, std::string processName)
{
	while (1) {
		InjectNewProcessInfo args;
		args.dllPath = dll;
		args.map = &mapper;
		args.processName = processName;

		EnumWindows(__windowCallback, (LPARAM)&args);
		m_lastWnds = m_newWnds;
		m_newWnds.clear();
		isFirstTime = false;
		Sleep(1500);
	}
	return false;
}

bool CInjector::AutoInjectProcess(CDllMap& mapper, bool stealHandleJob, std::string dll, std::string processName)
{
	HANDLE h = 0;
	DEBUG_LOG("Starting auto injection into process dll=%s processName=%s", dll.c_str(), processName.c_str());

	if (stealHandleJob) {
		auto pid = 0UL;
		auto desk_hwnd = GetShellWindow(); //auto desk_hwnd = FindWindow(0, L"Steam"); for steam games
		auto ret = GetWindowThreadProcessId(desk_hwnd, &pid);
		auto exp_handle = OpenProcess(PROCESS_ALL_ACCESS, true, pid);
		auto io_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
		auto job_object = CreateJobObjectW(0, 0);
		auto job_io_port = JOBOBJECT_ASSOCIATE_COMPLETION_PORT{ 0, io_port };
		auto result = SetInformationJobObject(job_object, JobObjectAssociateCompletionPortInformation, &job_io_port, sizeof(job_io_port));
		result = AssignProcessToJobObject(job_object, exp_handle);

		ASAPInjectionInfo args;
		args.io_port = io_port;
		args.dllPath = dll;
		args.map = &mapper;
		args.moduleName = processName;


		DEBUG_LOG("Waiting for multiple processes");
		HANDLE threadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)__handleRecieverASAPNoReturn, &args, 0, 0);

		CloseHandle(exp_handle);
		if (threadHandle) {
			DWORD wait = WaitForSingleObject(threadHandle,-1);
			if (wait == WAIT_OBJECT_0) {
				CloseHandle(threadHandle);
				return 1;
			}
			else {
				ERROR_LOG("Error on WaitForSingleObject value_return=%#x", wait);
				CloseHandle(threadHandle);
				return 0;
			}
		}

	}
	else {
		InjectNewProcessInfo args;
		args.dllPath = dll;
		args.map = &mapper;
		args.processName = processName;

		DEBUG_LOG("Waiting for multiple processes using EnumProcesses");
		HANDLE threadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)__InjectNewProcess, &args, 0, 0);

		if (threadHandle)
			DWORD wait = WaitForSingleObject(threadHandle, -1);
	}

	return true;
}

bool CInjector::StartProcessAndInject(CDllMap& mapper, bool stealHandleJob, std::string file, std::string dll)
{
	HANDLE h = 0;
	DEBUG_LOG("Creating and injecting into process dll=%s process=%s", dll.c_str(), file.c_str());

	if (stealHandleJob) {
		h = CreateProcessAndStealHandle(file);
		if (h == 0) {
			return false;
		}
	}
	else {
		STARTUPINFOA si = { 0 };
		PROCESS_INFORMATION pi = { 0 };

		std::string path = file.substr(0, file.find_last_of("\\/"));
		if (!CreateProcessA(file.c_str(), NULL, NULL, NULL, FALSE, NULL, NULL, path.c_str(), &si, &pi)) {
			ERROR_LOG("Could not create process error_code=%#x", GetLastError());
			return false;
		}
		h = pi.hProcess;
	}
	bool return_val= mapper.mapImage(h, dll);
	CloseHandle(h);
	return return_val;
}

bool CInjector::InjectFromPID(CDllMap& mapper,int pid, std::string dll)
{
	DEBUG_LOG("Injecting into process dll=%s pid=%d", dll.c_str(), pid);

	HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, true, (DWORD)pid);
	if (!h) {
		DEBUG_LOG("Fail to get handle to process");
	}

	return mapper.mapImage(h, dll);
}

bool CInjector::InjectFromClickingWindow(CDllMap& mapper, std::string dll)
{
	printf("Not implemented");
	return false;
}


HANDLE CInjector::CreateProcessAndStealHandle(std::string& file)
{
	auto pid = 0UL;
	//auto desk_hwnd = GetShellWindow(); //auto desk_hwnd = FindWindow(0, L"Steam"); for steam games
	auto ret = GetCurrentProcessId();//GetWindowThreadProcessId(desk_hwnd, &pid);
	auto exp_handle = OpenProcess(PROCESS_ALL_ACCESS, true, ret);
	auto io_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
	auto job_object = CreateJobObjectW(0, 0);
	auto job_io_port = JOBOBJECT_ASSOCIATE_COMPLETION_PORT{ 0, io_port };
	auto result = SetInformationJobObject(job_object, JobObjectAssociateCompletionPortInformation, &job_io_port, sizeof(job_io_port));
	result = AssignProcessToJobObject(job_object, exp_handle);

	ASAPArgs args;
	args.io_port = io_port;
	args.returnHandle = 0;
	args.moduleName = file;

	auto threadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)__handleRecieverASAP, &args, 0, 0);

	STARTUPINFOA si = { 0 };
	PROCESS_INFORMATION pi = { 0 };

	std::string path = file.substr(0, file.find_last_of("\\/"));
	if (!CreateProcessA(file.c_str(), NULL, NULL, NULL, FALSE, NULL, NULL, path.c_str(), &si, &pi)) {
		ERROR_LOG("Could not create process error_code=%#x", GetLastError());
		CloseHandle(exp_handle);
		return 0;
	}


	//auto threadHandle = CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)__handleRecieverASAP, &args, 0, 0);
	if (threadHandle) {
		DWORD wait = WaitForSingleObject(threadHandle, 10000);
		if (wait == WAIT_OBJECT_0) {
			CloseHandle(threadHandle);
			CloseHandle(exp_handle);
			return args.returnHandle;
		}
		else {
			ERROR_LOG("Error on WaitForSingleObject value_return=%#x", wait);
			CloseHandle(threadHandle);
			CloseHandle(exp_handle);
			return 0;
		}
	}
	else {
		ERROR_LOG("Error creating thread error_code=%#x", GetLastError());
	}
	CloseHandle(exp_handle);
	return 0;
}


void CInjector::__InjectNewProcess(InjectNewProcessInfo* args)
{
	// Get the list of process identifiers.
	std::string processName = args->processName;
	static bool itsFirsTime = true; //Used to not inject in processes already in memory
	while (1) {
		DWORD aProcesses[1024], cbNeeded, cProcesses;
		unsigned int i;

		if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
		{
			DEBUG_LOG("Could not enumerate current processes");
			continue;
		}
		// Calculate how many process identifiers were returned.
		cProcesses = cbNeeded / sizeof(DWORD);
		char buffer[MAX_PATH];
		std::set<int> this_execution;

		for (i = 0; i < cProcesses; i++){
			int pid = aProcesses[i];
			if (pid != 0){

				//Check if already existed in set
				this_execution.insert(pid);
				if (m_lastProcesses.count(pid) == 1) {
					continue;
				}
				//Inject if process name match
				auto race_handle = OpenProcess(PROCESS_ALL_ACCESS, true, (DWORD)pid);
				if (race_handle) {
					GetModuleFileNameExA(race_handle, 0, buffer, MAX_PATH);
					std::string currProcessName = getFileNameFromPath(buffer);
					DEBUG_LOG("New Process Detected : %s, PID:%d", currProcessName.data(), pid);
					if (processName.compare(currProcessName) == 0 && itsFirsTime==false) {
						bool result = args->map->mapImage(race_handle, args->dllPath);
						DEBUG_LOG("Injected into : %s, PID:%d, Result Code:%d", buffer, pid, result);
					}
					CloseHandle(race_handle);
				}
			}
		}
		itsFirsTime = false;

		//Replace for new set
		m_lastProcesses = this_execution;
		Sleep(1500);//Run every 1.5s
	}
}

BOOL CALLBACK CInjector::__windowCallback(HWND hWnd, LPARAM lParam)
{
		//Inject
	InjectNewProcessInfo* args = (InjectNewProcessInfo*)lParam;
	DWORD wId = 0;
	GetWindowThreadProcessId((HWND)hWnd,&wId);
	if (wId) {
		m_newWnds.insert(wId);

		if (m_lastWnds.count(wId)) {
			return TRUE;
		}

		//Avoid current windows
		if (isFirstTime) {
			return true;
		}
		auto pHandle = OpenProcess(PROCESS_ALL_ACCESS, true, (DWORD)wId);
		if (!pHandle) {
			DEBUG_LOG("Error opening process ID:%d", wId);
			return TRUE;
		}
		char buffer[MAX_PATH];
		GetModuleFileNameExA(pHandle, 0, buffer, MAX_PATH);
		std::string currProcessName = getFileNameFromPath(buffer);

		DEBUG_LOG("New window created by process: %s, PID:%d", currProcessName.data(), wId);
		if (currProcessName.compare(args->processName) == 0 && !m_injectedProcesses.count(wId)) {

			if (args->map->mapImage(pHandle, args->dllPath)) {
				DEBUG_LOG("Success injecting into ID: %s, PID:%d", currProcessName.data(), wId);
				m_injectedProcesses.insert(wId);
			}
			else {
				DEBUG_LOG("Error injecting into ID: %s, PID:%d", currProcessName.data(), wId);
			}
		}
	}
	return TRUE;
}


void CInjector::__handleRecieverASAP(ASAPArgs* args)
{
	DWORD nOfBytes;
	ULONG_PTR cKey;
	LPOVERLAPPED pid;
	char buffer[MAX_PATH];
	while (GetQueuedCompletionStatus(args->io_port, &nOfBytes, &cKey, &pid, -1)){
		if (nOfBytes == 6 ) {
			auto race_handle = OpenProcess(PROCESS_ALL_ACCESS, true, (DWORD)pid);
			GetModuleFileNameExA(race_handle, 0, buffer, MAX_PATH);
			if (args->moduleName.compare(buffer)==0) {
				args->returnHandle = race_handle;
				DEBUG_LOG("Stolen handle : %08x for %d\n", race_handle, pid);
				break;
			}
			CloseHandle(race_handle);
		}
	}
}

void CInjector::__handleRecieverASAPNoReturn(ASAPInjectionInfo* args)
{
	DWORD nOfBytes;
	ULONG_PTR cKey;
	LPOVERLAPPED pid;
	char buffer[MAX_PATH] = {0};
	while (GetQueuedCompletionStatus(args->io_port, &nOfBytes, &cKey, &pid, -1)) {
		if (nOfBytes == 6) {
			auto race_handle = OpenProcess(PROCESS_ALL_ACCESS, true, (DWORD)pid);
			if(race_handle){
				GetModuleFileNameExA(race_handle, 0, buffer, MAX_PATH);
				std::string processName = getFileNameFromPath(buffer);

				DEBUG_LOG("Process Started | Path:%s PID:%d\n", processName.data(), pid);
				if (args->moduleName.compare(processName) == 0) {
					args->map->mapImage(race_handle, args->dllPath);
					DEBUG_LOG("Injected into stolen handle : %08x for %d\n", race_handle, pid);
				}
				CloseHandle(race_handle);
			}
		}
	}
}

CManualMap::CManualMap(bool hijackThread): hijackThread(hijackThread)
{
}

bool CManualMap::mapImage(HANDLE h, std::string dll)
{
	std::string path = dll.substr(0, dll.find_last_of("\\/"));
	blackbone::Process prc;
	prc.Attach(h);
	blackbone::CustomArgs_t args;

	args.push_back(dll.data(),dll.size());
	auto flags = blackbone::NoFlags;
	//std::wcstombs()
	std::wstring process_path = CharToWchar(dll);

	auto addr = prc.mmap().MapImage(process_path, flags, nullptr, nullptr, &args);
	if (!addr) {
		std::string str = "Fail to inject " + dll + " into process with handle " + std::to_string((int)h) + "\n";
		str += "Status Code = " + std::to_string(addr.status) + "\n";
		std::string narrow = WcharToChar(blackbone::Utils::GetErrorDescription(addr.status));
		str += narrow;
		ERROR_LOG(str.c_str());
		return false;
	}
	return true;
}

CLoadLibrary::CLoadLibrary(bool hijackThread)
{
}

bool CLoadLibrary::mapImage(HANDLE h, std::string dll)
{
	DEBUG_LOG("Mapping with LoadLibraryA (%s)", dll.c_str());
	std::wstring wdll = CharToWchar(dll);
	blackbone::pe::PEImage pe;
	blackbone::Process process;
	process.Attach(h);
	BOOL is_x32;
	if (!IsWow64Process(h, &is_x32)) {
		ERROR_LOG("Error determining IsWow64Process, error code:%#x", GetLastError());
		return false;
	}

	void* LoadLibraryAddr = 0;
	void* GetLastErrorAddr = 0;
	if (is_x32) {
		__int64 kernelBase = 0;

		auto dataPtr = process.modules().GetModule(L"kernel32.dll", blackbone::eModSeachType::LdrList, blackbone::eModType::mt_mod32);
		if (!dataPtr) {
			ERROR_LOG("Error cannot find kernel32.dll");
			return false;
		}
		kernelBase = dataPtr.get()->baseAddress;
		

		//DEBUG_LOG("Kernel BaseAddress %#x", kernelBase);

		//Find LoadLibraryA and GetLastError
		NTSTATUS loaded = pe.Load(L"C:\\Windows\\SysWOW64\\kernel32.dll");
		if (loaded != STATUS_SUCCESS) {
			ERROR_LOG("Error loading kernel32.dll, error code:%#x", loaded);
			return false;
		}

		/*if (pe.Parse() != STATUS_SUCCESS) {
			ERROR_LOG("Error parsing kernel32.dll");
			return false;
		}*/

		blackbone::pe::vecExports exports;
		pe.GetExports(exports);

		for (auto& _export : exports) {
			if (_export.name.compare("LoadLibraryA") == 0) {
				LoadLibraryAddr = (void*)((__int64)_export.RVA + kernelBase);
			}
			if (_export.name.compare("GetLastError") == 0) {
				GetLastErrorAddr = (void*)((__int64)_export.RVA + kernelBase);
			}
		}

		if (!LoadLibraryAddr) {
			ERROR_LOG("Error fail to find LoadLibraryA in kernel32.dll");
			return false;
		}

		if (!GetLastErrorAddr) {
			ERROR_LOG("Error fail to find GetLastError in kernel32.dll");
			return false;
		}
	}
	else {
		LoadLibraryAddr = LoadLibraryA;
		GetLastErrorAddr = GetLastError;
	}

	//DEBUG_LOG("LoadLibrary at %#x, GetLastError at %#x", LoadLibraryAddr, GetLastErrorAddr);

	// Allocate space in the process for our DLL
	LPVOID memory = LPVOID(VirtualAllocEx(h, nullptr, dll.size(), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
	if (!memory) {
		ERROR_LOG("Error allocating memory, error code:%#x", GetLastError());
		return false;
	}
	// Write the string name of our DLL in the memory allocated 
	if (!WriteProcessMemory(h, memory, dll.data(), dll.size(), nullptr)) {
		ERROR_LOG("Error writting memory, error code:%#x", GetLastError());
		return false;
	}

	// Load our DLL
	HANDLE hThread = CreateRemoteThread(h, nullptr, NULL, LPTHREAD_START_ROUTINE(LoadLibraryAddr), memory, NULL, nullptr);
	if (!hThread) {
		ERROR_LOG("Error creating thread, error code:%#x", GetLastError());
		return false;
	}
	DWORD wait = WaitForSingleObject(hThread, 10000);
	if (wait == WAIT_OBJECT_0) {
		DWORD return_value;
		if (GetExitCodeThread(hThread, &return_value)) {
			if (return_value == 0) {
				HANDLE hThread2 = CreateRemoteThread(h, nullptr, NULL, LPTHREAD_START_ROUTINE(GetLastErrorAddr),NULL, NULL, nullptr);
				DWORD last_error_value = 0;
				if (hThread2 && WaitForSingleObject(hThread2, 10000) == WAIT_OBJECT_0 && GetExitCodeThread(hThread2, &last_error_value)) {
					ERROR_LOG("LoadLibrary return 0, error code:%#x", last_error_value);
					CloseHandle(hThread);
					CloseHandle(hThread2);
					return false;
				}

				ERROR_LOG("LoadLibrary return 0, couldn't get error code");
				CloseHandle(hThread);
				CloseHandle(hThread2);
				return false;
			}
		}
		CloseHandle(hThread);
	}
	else {
		ERROR_LOG("LoadLibrary thread timed out, error code:%#x", wait);
	}
	DEBUG_LOG("DLL loaded sucessfully!");
	return true;
}
