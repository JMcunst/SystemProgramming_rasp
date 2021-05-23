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

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH   1

#define POUT    23
#define PIN     24
#define PWM     0

double distance;

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
		
		printf("write value!\n");
		
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
        fprintf(stderr, "Failed to open in enable!\n");
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

void *ultrawave_thd(){
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

        time=(double)(end_t-start_t)/CLOCKS_PER_SEC;
        distance=time/2*34000;
        if(distance>900){
            distance=900;
        }
        printf("time: %.4lf\n",time);
        printf("distance: %.2lfcm\n",distance);
        usleep(500000);
    }
}

void *led_thd(){
    int target_bright=0;
    int prev_bright=0;
    int period = 12000000;

    PWMExport(PWM);
    usleep(100000);
    PWMWritePeriod(PWM,period);
    usleep(100000);
    PWMWriteDutyCycle(PWM,0);
    usleep(100000);
    PWMEnable(PWM);
    usleep(100000);

    while(1){
        target_bright=(int) period/(distance/2);
        PWMWriteDutyCycle(PWM,target_bright);
        usleep(50000);
    }
    exit(0);
}

static int PWMUnexport(int pwmnum) {
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;

    fd=open("/sys/class/pwm/pwmchip0/unexport",O_WRONLY);
    if(-1==fd){
        fprintf(stderr,"failed to open in unexport pwm\n");
        return -1;
    }

    bytes_written=snprintf(buffer,BUFFER_MAX,"%d",pwmnum);
    write(fd,buffer,bytes_written);
    close(fd);

    sleep(1);
    return 0;
}

int main(void){

    if(-1==GPIOUnexport(POUT)||-1==GPIOUnexport(PIN) || PWMUnexport(PWM)){
        return -1;
    }
    
    pthread_t p_thread[2];
    int thr_id;
    int status;

    char p1[] = "thread_1";
    char p2[] = "thread_2";

    thr_id = pthread_create(&p_thread[0], NULL, ultrawave_thd, NULL);
    if(thr_id < 0){
        perror("thread create error : ");
        exit(0);
        usleep(1000);
    }
    thr_id = pthread_create(&p_thread[1], NULL, led_thd, NULL);
    if(thr_id < 0){
        perror("thread create error : ");
        exit(0);
        usleep(1000);
    }

    usleep(1000000*10);
    
    if(-1==GPIOUnexport(POUT)||-1==GPIOUnexport(PIN)){
        return -1;
    }

    PWMUnexport(PWM);

    if(status=pthread_cancel(p_thread[0])!=0){
        perror("thread can't cancel:");
        exit(0);
    }
    if(status=pthread_cancel(p_thread[1])!=0){
        perror("thread can't cancel:");
        exit(0);
    }

    

    return 0;
}