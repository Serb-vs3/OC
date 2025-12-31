#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <dirent.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

bool kill_pid(pid_t pid){return kill(pid,SIGTERM)==0;}

std::string read_comm(pid_t pid){
    std::string path="/proc/"+std::to_string(pid)+"/comm";
    FILE* f=fopen(path.c_str(),"r");
    if(!f)return "";
    char buf[256];
    if(!fgets(buf,sizeof(buf),f)){fclose(f);return "";}
    fclose(f);
    std::string s(buf);
    if(!s.empty()&&s.back()=='\n')s.pop_back();
    return s;
}

void kill_by_name(const std::string& name){
    DIR* dir=opendir("/proc");
    if(!dir)return;
    struct dirent* ent;
    while((ent=readdir(dir))!=nullptr){
        if(!isdigit(ent->d_name[0]))continue;
        pid_t pid=atoi(ent->d_name);
        if(read_comm(pid)==name)kill_pid(pid);
    }
    closedir(dir);
}

int main(int argc,char* argv[]){
    std::vector<std::string> args(argv+1,argv+argc);
    for(size_t i=0;i<args.size();++i){
        if(args[i]=="--id"&&i+1<args.size())kill_pid(static_cast<pid_t>(std::stol(args[i+1])));
        if(args[i]=="--name"&&i+1<args.size())kill_by_name(args[i+1]);
    }
    const char* env=getenv("PROC_TO_KILL");
    if(env){
        std::string s(env);
        size_t pos;
        while((pos=s.find(','))!=std::string::npos){
            kill_by_name(s.substr(0,pos));
            s.erase(0,pos+1);
        }
        if(!s.empty())kill_by_name(s);
    }
    return 0;
}