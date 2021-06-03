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

#define POUT    23
#define PIN     24
#define PWM     0

double distance;
double queue_array[QUEUE_MAX] = {,0};
int rear = -1;
int front = -1;
int queueCycleCount = 0;  
int realCycleCount = 0;

void customInsertQueue(double qDist)
{
	queueCycleCount++;
    if(qDist >= 500.0 ){  //noise condition
        printf("qDist is over 100.0 cm. Insert is failed.\n");
        return;
    }
    if (rear == QUEUE_MAX - 1){   //queue is full
        printf("Queue Overflow n");
		double sum = 0.0;
		for(int i = 0; i < 10; i++){
			sum += queue_array[i];
		}
		sum = sum/10.0;
		distance = sum;
		// TODO : qDist를 계산하는 알고리즘
		
        refreshQueue();
        front = 0;
        rear = 0;
        queue_array[rear] = qDist;
    }
	else
    {
        if (front == -1)
            front = 0;
        printf("Inset the element in queue : ");
        rear = rear + 1;
        queue_array[rear] = qDist;
		realCycleCount++;
    }
	if(queueCycleCount == 10){
		double sum = 0.0;
		for(int i = 0; i < realCycleCount; i++){
			sum += queue_array[i];
		}
		sum = sum / (double)realCycleCount;
		distance = sum;
	}
}

void customRefreshQueue(){
    memset(queue_array, 0.0, QUEUE_MAX * sizeof(double));
}

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
    clock_t start_t,end_t;
    double time;

    if(-1==GPIOExport(POUT)||-1==GPIOExport(PIN)){
        printf("gpio export err ultra\n");
        exit(0);
    }

    usleep(100000);

    if(-1==GPIODirection(POUT,OUT)||-1==GPIODirection(PIN,IN)){
        printf("gpio direction err ultra\n");
        exit(0);
    }

    GPIOWrite(POUT,0);
    usleep(10000);

    while(1){
        if(-1==GPIOWrite(POUT,1)){
            printf("gpio write/trigger err\n");
            exit(0);
        }

        usleep(10);
        GPIOWrite(POUT,0);

        while(GPIORead(PIN)==0){
            start_t=clock();
        }
        while(GPIORead(PIN)==1){
            end_t=clock();
        }
        printf("--------------------------\n");
        time = (double)(end_t-start_t)/CLOCKS_PER_SEC;
        distance = time/2*34000;
		customInsertQueue(distance);

        if(distance > 500.0){
            distance = 500.0;
        }
		if(queueCycleCount == 10){
        	//printf("time: %.4lf\n",time);
        	printf("distance: %.2lfcm\n",distance);
        	usleep(500000);
		}
    }
}