#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define PIN 18
#define POUT 17
#define PBTN 21
#define PBTN2 20

#define BUFFER_MAX 3
#define DIRECTION_MAX 45
#define VALUE_MAX 64

/*Export Export Export Export*/
static int GPIOExport(int pin){

    char buffer[BUFFER_MAX];
    ssize_t bytes_written; 
    int fd;

    fd = open("/sys/class/gpio/export", O_WRONLY);
    if(-1 == fd){
        fprintf(stderr, "Failed to open export for writing!\n");
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return(0);
}

/*Direction Direction Direction Direction*/
static int GPIODirection(int pin, int dir){
    static const char s_directions_str[] = "in\0out";

    char path[DIRECTION_MAX] = "/sys/class/gpio/gpio24/direction";
    int fd;

    snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
    
    fd = open(path, O_WRONLY);
    if( -1 == fd){
        fprintf(stderr, "Failed to open gpio direction for writing!\n");
        return(-1);
    }
    if(-1 == write(fd,&s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)){
        printf(stderr, "Failed to set direction!\n");
        return(-1);
    }

    close(fd);
    return(0);
}

/*Unexport Unexport Unexport Unexport*/
static int GPIOUnexport(int pin){
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;

    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if(-1 == fd){
        fprintf(stderr, "Failed to open unexport for writing!\n");
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return(0);
}
/*Read Read Read Read Read*/
static GPIORead(int pin){

    char path[VALUE_MAX];
    char value_str[3];
    int fd;

    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_RDONLY);
    if(-1 == fd){
        fprintf(stderr, "Failed to open gpio value for reading!\n");
        return(-1);
    }
    if(-1 == read(fd,value_str,3)){
        fprintf(stderr, "Failed to read value!\n");
        return(-1);
    }

    close(fd);

    return(atoi(value_str));
}
/*Write Write Write Write*/
static int GPIOWrite(int pin, int value){
    static const char s_values_str[] = "01";

    char path[VALUE_MAX];
    int fd;

    printf("write value!\n");

    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_WRONLY);
    if(-1 == fd){
        fprintf(stderr, "Failed to open gpio value for writing!\n");
        return(-1);
    }
    if(1!=write(fd, &s_values_str[LOW == value ? 0 : 1], 1)){
        fprintf(stderr, "Failed to write value!\n");
        return(-1);

        close(fd);
        return(0);
    }
    printf("write value!!!!!\n");
}

// #define IN 0
// #define OUT 1
// #define LOW 0
// #define HIGH 1
// #define PIN 18
// #define POUT 17
// #define PBTN 21
// #define PBTN2 20

int main(int argc, char **argv){

    int repeat = 100;
    int btnOn = 0;
    int state = 0;
    int isChanged = 0;

    if(-1 == GPIOExport(POUT) || -1 == GPIOExport(PBTN) || -1 == GPIOExport(PBTN2))
        return(1);
    sleep(1);
    if(-1 == GPIODirection(POUT, OUT) || -1 == GPIODirection(PBTN, OUT) || -1 == GPIODirection(PBTN2, IN))
        return(2);
    sleep(1);
    GPIOWrite(PBTN, 1);
    
    do {
        btnOn = GPIORead(PBTN2);
        if(btnOn == 1){ // btn off
            if(-1 == GPIOWrite(POUT, state)){
                return(3);
            }
            isChanged = 0;
            usleep(100*1000);
        }else{          // btn push
            if(!isChanged){
                isChanged = 1;
                if(state == 0){                    
                    state = 1;
                }
                else
                {
                    state = 0;
                }
            }
            usleep(100*1000);
        }
        
    }while(repeat--);
    
    if(-1 == GPIOUnexport(PBTN) || -1 == GPIOUnexport(PBTN2) || -1 == GPIOUnexport(POUT))
        return(4);

    return(0);
}