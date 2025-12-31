#include <windows.h>
#include <iostream>
#include <string>

void launch(const std::wstring& exe,PROCESS_INFORMATION& pi){
    STARTUPINFOW si{};si.cb=sizeof(si);
    CreateProcessW(nullptr,const_cast<wchar_t*>(exe.c_str()),nullptr,nullptr,FALSE,0,nullptr,nullptr,&si,&pi);
}

bool alive(DWORD pid){
    HANDLE h=OpenProcess(SYNCHRONIZE,FALSE,pid);
    if(!h)return false;DWORD code=0;
    bool a=GetExitCodeProcess(h,&code)&&code==STILL_ACTIVE;
    CloseHandle(h);return a;
}

void run_cmd(const std::wstring& cmd){
    STARTUPINFOW si{};si.cb=sizeof(si);PROCESS_INFORMATION pi{};
    CreateProcessW(nullptr,const_cast<wchar_t*>(cmd.c_str()),nullptr,nullptr,FALSE,0,nullptr,nullptr,&si,&pi);
    WaitForSingleObject(pi.hProcess,INFINITE);
    CloseHandle(pi.hProcess);CloseHandle(pi.hThread);
}

int wmain(){
    SetEnvironmentVariableW(L"PROC_TO_KILL",L"notepad.exe");
    PROCESS_INFORMATION pi{};
    launch(L"notepad.exe",pi);Sleep(1000);
    std::wcout<<L"Process "<<pi.dwProcessId<<L" alive\n";
    run_cmd(L"Killer.exe --id "+std::to_wstring(pi.dwProcessId));
    std::wcout<<L"Alive after --id? "<<alive(pi.dwProcessId)<<L"\n";
    PROCESS_INFORMATION pi2{};
    launch(L"notepad.exe",pi2);Sleep(1000);
    std::wcout<<L"Process with name notepad.exe alive\n";
    run_cmd(L"Killer.exe --name notepad.exe");
    std::wcout<<L"Alive after --name? "<<alive(pi2.dwProcessId)<<L"\n";
    PROCESS_INFORMATION pi3{};
    launch(L"notepad.exe",pi3);Sleep(1000);
    std::wcout<<L"Process with env var alive\n";
    run_cmd(L"Killer.exe");
    std::wcout<<L"Alive after env kill? "<<alive(pi3.dwProcessId)<<L"\n";
    SetEnvironmentVariableW(L"PROC_TO_KILL",nullptr);
    return 0;
}