#include "suspender.h"
#include <tlhelp32.h>
#include <cwchar>   // for _wcsicmp

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

void ThreadSuspender::SuspendProcessByName(const std::wstring& processName)
{
    DWORD targetPID = 0;

    // Find process ID by name
    HANDLE processSnapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS,
        0
    );

    if (processSnapshot == INVALID_HANDLE_VALUE)
        return;

    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(processSnapshot, &pe))
    {
        do
        {
            if (_wcsicmp(pe.szExeFile, processName.c_str()) == 0)
            {
                targetPID = pe.th32ProcessID;
                break;
            }

        } while (Process32NextW(processSnapshot, &pe));
    }

    CloseHandle(processSnapshot);


    if (targetPID == 0)
        return;


    // Suspend all threads belonging to that PID
    HANDLE threadSnapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPTHREAD,
        0
    );

    if (threadSnapshot == INVALID_HANDLE_VALUE)
        return;


    THREADENTRY32 te{};
    te.dwSize = sizeof(te);

    if (Thread32First(threadSnapshot, &te))
    {
        do
        {
            if (te.th32OwnerProcessID == targetPID)
            {
                HANDLE hThread = OpenThread(
                    THREAD_SUSPEND_RESUME,
                    FALSE,
                    te.th32ThreadID
                );

                if (hThread)
                {
                    if (SuspendThread(hThread) != (DWORD)-1)
                        suspendedThreads.push_back(hThread);
                    else
                        CloseHandle(hThread);
                }
            }

        } while (Thread32Next(threadSnapshot, &te));
    }

    CloseHandle(threadSnapshot);
}

void ThreadSuspender::ResumeAllProcess()
{
    for (HANDLE hThread : suspendedThreads)
    {
        ResumeThread(hThread);
        CloseHandle(hThread);
    }

    suspendedThreads.clear();
}