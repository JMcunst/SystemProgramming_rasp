#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

int main(){
    int fd1 = open("./dup_ex_data", O_CREAT|O_WRONLY, 0755);
    write(fd1, "Hello\n", strlen("Hello\n"));

    int fd2 = dup(fd1);
    write(fd1, "Hi\n", strlen("Hi\n"));
    close(fd1);
    
    write(fd2, "nihaoo\n", strlen("nihaoo\n"));
    close(fd2);

    return 0;
}