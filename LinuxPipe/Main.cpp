#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <vector>
#include <iostream>
int spawn(const char* prog,int in,int out){
    pid_t pid=fork();
    if(pid==0){
        if(in!=-1){dup2(in,0);}
        if(out!=-1){dup2(out,1);}
        execlp(prog,prog,nullptr);
        _exit(1);
    }
    return pid;
}
int main(){
    int p0[2],p1[2],p2[2],p3[2];
    pipe(p0);pipe(p1);pipe(p2);pipe(p3);
    std::vector<int> pids;
    pids.push_back(spawn("./M",p0[0],p1[1]));
    pids.push_back(spawn("./A",p1[0],p2[1]));
    pids.push_back(spawn("./P",p2[0],p3[1]));
    pids.push_back(spawn("./S",p3[0],-1));
    close(p0[0]);close(p1[0]);close(p2[0]);close(p3[0]);
    close(p1[1]);close(p2[1]);close(p3[1]);
    const char* data="1 2 3 4 5\n";
    write(p0[1],data,strlen(data));
    close(p0[1]);
    for(int pid:pids)waitpid(pid,nullptr,0);
    return 0;
}
