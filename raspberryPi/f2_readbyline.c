#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv){

    int fd;
    int rd;
    char buf[256];
    if(argc != 3){
        fprintf(stderr, "usage: %s <file name> <access mode>\n", argv[0]);
        return 0;
    }

    fd = open(argv[1], O_WRONLY|O_CREAT|O_APPEND,0777);
    if(fd == -1){
        perror("failed open for write");
        return 1;
    }

    write(fd, argv[2], strlen(argv[2]));
    write(fd, "\n", -1);

    close(fd);

    fd = open(argv[1], O_RDONLY, 0777);
    if(fd == -1){
        perror("failed open for read");
        return 1;
    }
    memset(buf, 0, sizeof(buf));

    while((rd = read(fd, buf, sizeof(buf)))>0){
        printf("%s\n",buf);
    }
    
    close(fd);

    return 0;
}