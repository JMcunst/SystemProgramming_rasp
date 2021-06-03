#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define IN  0
#define OUT 1
#define LOW  0
#define HIGH 1
#define PIN  20
#define POUT 21

void error_handling(char *message){
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

int main(int argc, char *argv[])
{
    int state = 1;
    int prev_state = 1;
    int light = 0;

    int serv_sock, clnt_sock = -1;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    char msg[2];
        
    //Enable GPIO pins
    if (-1 == GPIOExport(PIN) || -1 == GPIOExport(POUT))
        return (1);
    //Set GPIO directions
    if (-1 == GPIODirection(PIN, IN) || -1 == GPIODirection(POUT, OUT))
        return (2);
    if (-1 == GPIOWrite(POUT, 1))
        return (3);
        
    if (argc != 2)
    {
        printf("Usage : %s <port>\n", argv[0]);       
    }

    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");
            
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));
        
    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");
    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");
    if (clnt_sock < 0)
    {
        clnt_addr_size = sizeof(clnt_addr);
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr,
           & clnt_addr_size);
        if (clnt_sock == -1)
            error_handling("accept() error");       
    }
    while (1)
    { 
        state = GPIORead(PIN);
        if (prev_state == 0 && state == 1)
        {
            light = (light + 1) % 2;
            snprintf(msg, 2, "%d", light);
            write(clnt_sock, msg, sizeof(msg));
            printf("msg = %s\n", msg);
            close(clnt_sock);
            clnt_sock = -1;           
        }            
        prev_state = state;
        usleep(500 * 100);               
    }
        
    close(serv_sock);
        
    //Disable GPIO pins
    if (-1 == GPIOUnexport(PIN) || -1 == GPIOUnexport(POUT))
        return (4);
    return (0);
}