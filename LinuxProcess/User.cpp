#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <signal.h>

bool alive(pid_t p){return kill(p,0)==0;}

int main(){
    setenv("PROC_TO_KILL","sleep",1);
    pid_t p=fork();
    if(p==0){execlp("sleep","sleep","60",nullptr);_exit(1);}
    sleep(1);
    std::cout<<"Process "<<p<<" alive\n";
    std::string cmd="./Killer --id "+std::to_string(p);
    system(cmd.c_str());
    std::cout<<"Alive after --id? "<<alive(p)<<"\n";
    pid_t p2=fork();
    if(p2==0){execlp("sleep","sleep","60",nullptr);_exit(1);}
    sleep(1);
    std::cout<<"Process with name sleep alive\n";
    system("./Killer --name sleep");
    std::cout<<"Alive after --name? "<<alive(p2)<<"\n";
    pid_t p3=fork();
    if(p3==0){execlp("sleep","sleep","60",nullptr);_exit(1);}
    sleep(1);
    std::cout<<"Process with env var alive\n";
    system("./Killer");
    std::cout<<"Alive after env kill? "<<alive(p3)<<"\n";
    unsetenv("PROC_TO_KILL");
    return 0;
}