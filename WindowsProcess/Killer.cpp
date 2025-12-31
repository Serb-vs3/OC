#include <windows.h>
#include <tlhelp32.h>
#include <string>
#include <vector>
#include <iostream>

void kill_pid(DWORD pid){
    HANDLE h=OpenProcess(PROCESS_TERMINATE,FALSE,pid);
    if(h){TerminateProcess(h,0);CloseHandle(h);}
}

void kill_by_name(const std::wstring& name){
    HANDLE snap=CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS,0);
    if(snap==INVALID_HANDLE_VALUE)return;
    PROCESSENTRY32W pe;pe.dwSize=sizeof(pe);
    if(Process32FirstW(snap,&pe)){
        do{if(name==pe.szExeFile)kill_pid(pe.th32ProcessID);}while(Process32NextW(snap,&pe));
    }
    CloseHandle(snap);
}

std::vector<std::wstring> split(const std::wstring& s,wchar_t d){
    std::vector<std::wstring> v;size_t b=0,p;
    while((p=s.find(d,b))!=std::wstring::npos){v.push_back(s.substr(b,p-b));b=p+1;}
    v.push_back(s.substr(b));return v;
}

int wmain(int argc,wchar_t* argv[]){
    for(int i=1;i<argc;++i){
        std::wstring a=argv[i];
        if(a==L"--id"&&i+1<argc)kill_pid(std::stoul(argv[++i]));
        else if(a==L"--name"&&i+1<argc)kill_by_name(argv[++i]);
    }
    wchar_t buf[4096];
    if(GetEnvironmentVariableW(L"PROC_TO_KILL",buf,4096)){
        std::wstring env(buf);
        for(auto& n:split(env,L','))kill_by_name(n);
    }
    return 0;
}