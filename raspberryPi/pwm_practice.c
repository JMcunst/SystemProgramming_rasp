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
#define VALUE_MAX 256

#define BUFFER_MAX 3
#define DIRECTION_MAX 45

static int PWMExport(int pwmnum){
    char buffer[BUFFER_MAX];
    int bytes_written;
    int fd;

    fd = open("/sys/class/pwm/pwmchip0/unexport", O_WRONLY);
    if(-1 == fd){
        fprintf(stderr, "Failed to open in unexport!\n");
        return(-1);
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
    write(fd, buffer, bytes_written);
    close(fd);

    sleep(1);
    fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
    if(-1 == fd){
        fprintf(stderr, "Failed to open in export!\n");
        return(-1);
    }
    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
    write(fd, buffer, bytes_written);
    close(fd);
    sleep(1);
    return(0);
}

static int PWMEnable(int pwmnum){
    static const char s_unenable_str[] = "0";
    static const char s_enable_str[] = "1";

    char path[DIRECTION_MAX];
    int fd;

    snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm%d/enable", pwmnum);
    fd = open(path, O_WRONLY);
    if(-1 == fd){
        fprintf(stderr, "Failed to open in enalbe!\n");
        return(-1);
    }

    write(fd, s_unenable_str, strlen(s_unenable_str));
    close(fd);

    fd = open(path, O_WRONLY);
    if(-1 == fd){
        fprintf(stderr, "Failed to open in enalbe!\n");
        return(-1);
    }

    write(fd, s_enable_str, strlen(s_enable_str));
    close(fd);
    return(0);
}

static int PWMWritePeriod(int pwmnum, int value){
    char s_values_str[VALUE_MAX];
    char path[VALUE_MAX];
    int fd,byte;

    snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/period", pwmnum);
    fd = open(path, O_WRONLY);
    if(-1 == fd){
        fprintf(stderr, "Failed to open in period!\n");
        return(-1);
    }

    byte = snprintf(s_values_str, VALUE_MAX, "%d", value);

    if(-1 == write(fd,s_values_str,byte)){
        fprintf(stderr, "Failed to write value in period!\n");
        return(-1);
    }

    close(fd);
    return(0);
}

static int PWMWriteDutyCycle(int pwmnum, int value){
    char path[VALUE_MAX];
    char s_values_str[VALUE_MAX];
    int fd, byte;

    snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/duty_cycle", pwmnum);
    fd = open(path, O_WRONLY);
    if(-1 == fd){
        fprintf(stderr, "Failed to open in duty_cycle!\n");
        return(-1);
    }

    byte = snprintf(s_values_str, VALUE_MAX, "%d", value);

    if(-1 == write(fd, s_values_str, byte)){
        fprintf(stderr, "Failed to write value! in duty_cycle\n");
        close(fd);
        return(-1);
    }

    close(fd);
    return(0);
}

int main(void){
    PWMExport(0); //pwm0 is gpio18
    PWMWritePeriod(0,5000000);
    PWMWriteDutyCycle(0,1000000);
    PWMEnable(0);

    int repeat = 100;

    clock_t start_t, end_t;
    double time;
    while(1){
        for(int i = 0; i < 1000; i++){
            PWMWriteDutyCycle(0,i*10000);
            usleep(1000);
        }
        for(int i = 1000; i > 0; i--){
            PWMWriteDutyCycle(0,i*10000);
            usleep(1000);
        }
    }
}