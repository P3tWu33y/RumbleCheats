#pragma once
#include <windows.h>
#include <vector>

class ThreadSuspender
{
private:
  std::vector<HANDLE> suspendedThreads;

public:
  // Suspend all threads in the current process except the one calling this function
  void SuspendAllExceptCurrent();

  // Resume all previously suspended threads
  void ResumeAll();

  // Suspend Or Resume process by id (externally)
  bool SuspendResumeProcess(const char* processName, bool suspend);


  typedef LONG(NTAPI* NtSuspendProcess)(IN HANDLE ProcessHandle);
  typedef LONG(NTAPI* NtResumeProcess)(IN HANDLE ProcessHandle);

  NtSuspendProcess pfnNtSuspendProcess = (NtSuspendProcess)GetProcAddress(GetModuleHandleW(L"ntdll"), "NtSuspendProcess");
  NtResumeProcess pfnNtResumeProcess = (NtResumeProcess)GetProcAddress(GetModuleHandleW(L"ntdll"), "NtResumeProcess");
};
