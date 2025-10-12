#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <vector>
#include <string>
#include <iostream>
int spawn(const std::wstring& cmd,HANDLE in,HANDLE out){
    SECURITY_ATTRIBUTES sa={sizeof(sa),nullptr,TRUE};
    STARTUPINFOW si{};
    PROCESS_INFORMATION pi{};
    si.cb=sizeof(si);
    si.dwFlags=STARTF_USESTDHANDLES;
    si.hStdInput=in;
    si.hStdOutput=out;
    si.hStdError=GetStdHandle(STD_ERROR_HANDLE);
    std::wstring command=cmd;
    wchar_t* buf=&command[0];
    CreateProcessW(nullptr,buf,nullptr,nullptr,TRUE,0,nullptr,nullptr,&si,&pi);
    CloseHandle(pi.hThread);
    return (int)pi.hProcess;
}
int wmain(){
    SECURITY_ATTRIBUTES sa={sizeof(sa),nullptr,TRUE};
    HANDLE p0_r,p0_w,p1_r,p1_w,p2_r,p2_w,p3_r,p3_w;
    CreatePipe(&p0_r,&p0_w,&sa,0);
    CreatePipe(&p1_r,&p1_w,&sa,0);
    CreatePipe(&p2_r,&p2_w,&sa,0);
    CreatePipe(&p3_r,&p3_w,&sa,0);
    std::vector<int> procs;
    procs.push_back(spawn(L"M.exe",p0_r,p1_w));
    procs.push_back(spawn(L"A.exe",p1_r,p2_w));
    procs.push_back(spawn(L"P.exe",p2_r,p3_w));
    procs.push_back(spawn(L"S.exe",p3_r,GetStdHandle(STD_OUTPUT_HANDLE)));
    CloseHandle(p0_r);CloseHandle(p1_r);CloseHandle(p2_r);CloseHandle(p3_r);
    CloseHandle(p1_w);CloseHandle(p2_w);CloseHandle(p3_w);
    const char* data="1 2 3 4 5\n";
    DWORD written;
    WriteFile(p0_w,data,(DWORD)strlen(data),&written,nullptr);
    CloseHandle(p0_w);
    for(HANDLE h:(std::vector<HANDLE>{(HANDLE)procs[0],(HANDLE)procs[1],(HANDLE)procs[2],(HANDLE)procs[3]})){
        WaitForSingleObject(h,INFINITE);
        CloseHandle(h);
    }
    return 0;
}
