#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define TOTALFORK 5
// void main(void){
//     pid_t pid;
//     pid = fork();
//     if(pid == -1){
//         printf("can't fork\n");
//         exit(0);
//     }
//     if(pid == 0){
//         printf("child process id: %d\n", getpid());
//         sleep(1);
//         exit(0);
//     }
//     else{
//         printf("parent process id: %d\n", getpid());
//         sleep(1);
//         exit(1);
//     }
//     return;
// }

void main(){
    pid_t pids[TOTALFORK];
    int runProcess = 0;
    int state;

    while(runProcess < TOTALFORK){
        pids[runProcess] = fork();
        if(pids[runProcess] < 0 ){
            return -1;
        }else if(pids[runProcess] == 0){
            printf("child process %ld is running. \n", (long)getpid());
            sleep(1);
            printf("%d process is terminated. \n", getpid());
            exit(0);
        }else{
            //wait(&state);
            printf("parent %ld, child %d. \n", (long)getpid(), pids[runProcess]);
        }
        runProcess++;
    }

    printf("parent process is terminated\n");
    return 0;
}