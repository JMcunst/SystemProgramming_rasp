#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#define BUFFER_MAX 45
#define DIRECTION_MAX 45
#define VALUE_MAX 256
#define QUEUE_MAX 10

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH   1

#define POUT_R_TRIG    23
#define PIN_R_ECHO     24
#define R_FLAG 1
#define FDCOUNT 3
#define FDFULLCOUNT 5

static int GPIOUnexport(int pin){
	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;
	
	fd=open("/sys/class/gpio/unexport",O_WRONLY);
	if(-1==fd){
		fprintf(stderr,"falied to pen unexport\n");
		return -1;
	}
	
	bytes_written=snprintf(buffer,BUFFER_MAX,"%d",pin);
	write(fd,buffer,bytes_written);
	close(fd);
	return 0;
}

static int GPIODirection(int pin,int dir){
		static const char s_directions_str[]="in\0out";
		
	
	char path[DIRECTION_MAX]="/sys/class/gpio/gpio%d/direction";
	int fd;
	
	snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction",pin);
	
	fd=open(path,O_WRONLY);
	if(-1==fd){
		fprintf(stderr,"Failed to open gpio direction for writing!\n");
		return -1;
	}
	
	if(-1==write(fd,&s_directions_str[IN == dir ? 0 :3], IN==dir ? 2:3)){
		fprintf(stderr,"failed to set direction!\n");
		return -1;
	}
	
	close(fd);
	return 0;
}

static int GPIOExport(int pin){

	char buffer[BUFFER_MAX];
	ssize_t bytes_written;
	int fd;
	
	fd=open("/sys/class/gpio/export",O_WRONLY);
	if(-1==fd){
		fprintf(stderr, "failed export wr");
		return 1;
	}
	bytes_written=snprintf(buffer,BUFFER_MAX, "%d", pin);
	write(fd,buffer,bytes_written);
	close(fd);
	return 0;
}


static int GPIOWrite(int pin, int value){
		static const char s_values_str[] ="01";
		
		char path[VALUE_MAX];
		int fd;
		
		//printf("write value!\n");
		
		snprintf(path,VALUE_MAX, "/sys/class/gpio/gpio%d/value",pin);
		fd=open(path,O_WRONLY);
		if(-1==fd){
			fprintf(stderr,"failed open gpio write\n");
			return -1;
		}
		
		//0 1 selection
		if(1!=write(fd,&s_values_str[LOW==value ? 0:1],1)){
			fprintf(stderr,"failed to write value\n");
			return -1;
		}
		close(fd);
		return 0;
}

static int GPIORead(int pin){
    char path[VALUE_MAX];
    char value_str[3];
    int fd;

    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value",pin);
    fd=open(path, O_RDONLY);
    if(-1==fd){
        fprintf(stderr,"failed to open gpio value for reading\n");
        return -1;
    }

    if(-1==read(fd,value_str,3)){
        fprintf(stderr,"failed to read val\n");
        return -1;
    }
    close(fd);

    return(atoi(value_str));
}

int main(){
    if(-1==GPIOUnexport(POUT_R_TRIG)||-1== GPIOUnexport(PIN_R_ECHO)){
        return -1;
    }
    clock_t start_t,end_t;
    double time;
    double distance;

    int power;

    if(-1==GPIOExport(POUT_R_TRIG)||-1==GPIOExport(PIN_R_ECHO)){
        printf("gpio export err ultra\n");
        exit(0);
    }

    usleep(100000);

    if(-1==GPIODirection(POUT_R_TRIG,OUT) ||-1==GPIODirection(PIN_R_ECHO,IN)){
        printf("gpio direction err ultra\n");
        exit(0);
    }

    GPIOWrite(POUT_R_TRIG,0);
    usleep(10000);

    while(1){
        if(-1==GPIOWrite(POUT_R_TRIG,1)){
            printf("gpio Right write/trigger err\n");
            exit(0);
        }

        usleep(10);
        GPIOWrite(POUT_R_TRIG,0);
        printf("------------------------------------\n");
        while(GPIORead(PIN_R_ECHO) == 0){
            start_t=clock();
        }
        while(GPIORead(PIN_R_ECHO) == 1){
            end_t=clock();
        }

        printf("start : %f\n",start_t);
        printf("end : %f\n",end_t);
        printf("end - start : %f\n",end_t-start_t);

        time = (double)(end_t-start_t)/CLOCKS_PER_SEC;
        distance = time/2*34000;
        printf("time = %.5lf, ", time);
        printf("distance = %.2lfcm\n",distance);

        if(distance < 30){
            power = 2;
            
        }else if(distance < 100){
            power = 1;
        }else{
            power = 0;
        }
        if(power != 0){
            printf("power : %d\n",power);
        }else{
            printf("power : no change!! \n");
        }
        

        usleep(50000);
    }

    if(-1==GPIOUnexport(POUT_R_TRIG)||-1==GPIOUnexport(PIN_R_ECHO)){
        return -1;
    }

	return 0;
}