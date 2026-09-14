#include "suspender.h"
#include <tlhelp32.h>
#include <iostream>




void ThreadSuspender::SuspendAllExceptCurrent()
{
  DWORD currentProcessId = GetCurrentProcessId();
  DWORD currentThreadId = GetCurrentThreadId();

  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (snapshot == INVALID_HANDLE_VALUE)
    return;

  THREADENTRY32 te;
  te.dwSize = sizeof(te);

  if (Thread32First(snapshot, &te))
  {
    do
    {
      if (te.th32OwnerProcessID == currentProcessId && te.th32ThreadID != currentThreadId)
      {
        HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
        if (hThread)
        {
          if (SuspendThread(hThread) != (DWORD)-1)
          {
            suspendedThreads.push_back(hThread); // save handle to resume later
          }
          else
          {
            CloseHandle(hThread); // cleanup if suspend fails
          }
        }
      }
    } while (Thread32Next(snapshot, &te));
  }

  CloseHandle(snapshot);
}

void ThreadSuspender::ResumeAll()
{
  for (HANDLE hThread : suspendedThreads)
  {
    ResumeThread(hThread);
    CloseHandle(hThread); // release handle
  }
  suspendedThreads.clear();
}

// Add this new function
bool ThreadSuspender::SuspendResumeProcess(const char* processName, bool suspend)
{
    if (!processName)
        return false;

    // Create snapshot of all running processes
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to create process snapshot.\n";
        return false;
    }

    PROCESSENTRY32 pe32{};
    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hSnapshot, &pe32))
    {
        CloseHandle(hSnapshot);
        std::cerr << "Process32First failed.\n";
        return false;
    }

    DWORD processId = 0;

    do
    {
        if (_stricmp(pe32.szExeFile, processName) == 0)
        {
            processId = pe32.th32ProcessID;
            break;
        }

    } while (Process32Next(hSnapshot, &pe32));

    CloseHandle(hSnapshot);

    if (processId == 0)
    {
        std::cerr << "Process not found: " << processName << std::endl;
        return false;
    }

    HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, processId);
    if (!hProcess)
    {
        std::cerr << "Failed to open process. Error code: "
            << GetLastError() << std::endl;
        return false;
    }

    LONG result;
    if (suspend) {
        result = pfnNtSuspendProcess(hProcess);
    }
    else {
        result = pfnNtResumeProcess(hProcess);
    }

    CloseHandle(hProcess);
    return result == 0;
}