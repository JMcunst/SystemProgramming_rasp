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
#define POUT 17 
#define VALUE_MAX 30

void error_handling(char *message)
{
    fputs(message,stderr);
    fputc('\n',stderr);
    exit(1);
}

int main(int argc, char *argv[]) 
{
    int sock;
    struct sockaddr_in serv_addr;
    char msg[2];
    char on[2]="1";
    int str_len;
    int light = 0;
    if(argc!=3)
    {
        printf("Usage : %s <IP> <port>\n",argv[0]);
        exit(1);
    }
    //Enable GPIO pins
    if (-1 == GPIOExport(POUT))
        return(1);
    //Set GPIO directions
    if (-1 == GPIODirection(POUT, OUT))
        return(2);
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1)
        error_handling("socket() error");
        
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));  
    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr))==-1)
        error_handling("connect() error");
    while(1)
    {                    
        str_len = read(sock, msg, sizeof(msg));
        if(str_len == -1)
            error_handling("read() error");     
        printf("Receive message from Server : %s\n",msg);
        if(strncmp(on,msg,1)==0)
            light = 1;
        else
            light = 0;          
        GPIOWrite(POUT, light);      
    }
    close(sock);
    //Disable GPIO pins
    if (-1 == GPIOUnexport(POUT))
        return(4);
    return(0);
}