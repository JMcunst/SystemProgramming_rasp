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
#define QUEUE_MAX 15

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH   1

#define POUT_R_TRIG    23
#define PIN_R_ECHO     24

#define FLAG_R 1
#define FLAG_L 2

double qr_array[QUEUE_MAX] = {0,};
double ql_array[QUEUE_MAX] = {0,};
int rrear = -1;
int rfront = -1;
int lrear = -1;
int lfront = -1;
double mr_distance = 0.0;
double ml_distance = 0.0;

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
    //printf("<<<<<<< %d >>>>>>>>\n",atoi(value_str));
    return(atoi(value_str));
}
void customInsertQueue(double qDist, int qLR)
{
    if(qLR == FLAG_R){
        if (rrear == QUEUE_MAX - 1){ // Queue is Full
            printf("Right Queue is Full.\n");

            // Bubble Sort
            int temp;
            for (int i = 0; i < QUEUE_MAX; i++){    // 요소의 개수만큼 반복
                for (int j = 0; j < QUEUE_MAX - 1; j++){   // 요소의 개수 - 1만큼 반복
                    if (qr_array[j] > qr_array[j + 1]){          // 현재 요소의 값과 다음 요소의 값을 비교하여 큰 값을
                        temp = qr_array[j];
                        qr_array[j] = qr_array[j + 1];
                        qr_array[j + 1] = temp;            // 다음 요소로 보냄
                    }
                }
            }
            // Median Algorithm
            mr_distance = qr_array[QUEUE_MAX/2];

            //Queue Reset
            memset(qr_array, 0.0, QUEUE_MAX * sizeof(double));
            rfront = 0;
            rrear = 0;
            qr_array[rrear] = qDist;
        }
        else
        {
            mr_distance = 0;
            if (rfront == -1)
                rfront = 0;
            //printf("Insert the element in Queue : %.2lfcm\n ",qDist);
            rrear = rrear + 1;
            qr_array[rrear] = qDist;
        }
    }else{  // Left Sensor
        if (lrear == QUEUE_MAX - 1){ // Queue is Full
            printf("Left Queue is Full.\n");

            // Bubble Sort
            int temp;
            for (int i = 0; i < QUEUE_MAX; i++){    // 요소의 개수만큼 반복
                for (int j = 0; j < QUEUE_MAX - 1; j++){   // 요소의 개수 - 1만큼 반복
                    if (ql_array[j] > ql_array[j + 1]){          // 현재 요소의 값과 다음 요소의 값을 비교하여 큰 값을
                        temp = ql_array[j];
                        ql_array[j] = ql_array[j + 1];
                        ql_array[j + 1] = temp;            // 다음 요소로 보냄
                    }
                }
            }
            // Median Algorithm
            ml_distance = ql_array[QUEUE_MAX/2];

            //Queue Reset
            memset(ql_array, 0.0, QUEUE_MAX * sizeof(double));
            lfront = 0;
            lrear = 0;
            ql_array[lrear] = qDist;
        }
        else
        {
            ml_distance = 0;
            if (lfront == -1)
                lfront = 0;
            //printf("Insert the element in Queue : %.2lfcm\n ",qDist);
            lrear = lrear + 1;
            ql_array[lrear] = qDist;
        }
    }
}
int main(){
    if(-1==GPIOUnexport(POUT_R_TRIG)||-1== GPIOUnexport(PIN_R_ECHO)){
        return -1;
    }
    clock_t rstart_t,rend_t;
    clock_t lstart_t,lend_t;
    double rtime;
    double rdistance;
    double ltime;
    double ldistance;

    int rpower;
    int lpower;

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
        printf("step1\n");
        usleep(10);
        GPIOWrite(POUT_R_TRIG,0);
        printf("step2\n");
        //printf("------------------------------------\n");
        while(GPIORead(PIN_R_ECHO) == 0){
            rstart_t=clock();
        }
        printf("step3\n");
        while(GPIORead(PIN_R_ECHO) == 1){
            rend_t=clock();
        }
        

        //printf("start : %f\n",rstart_t);
        //printf("end : %f\n",end_t);
        //printf("end - start : %f\n",end_t-rstart_t);

        rtime = (double)(rend_t-rstart_t)/CLOCKS_PER_SEC;
        rdistance = rtime/2*34000;
        ltime = (double)(rend_t-rstart_t)/CLOCKS_PER_SEC;
        ldistance = ltime/2*34000;
        //printf("time = %.5lf, ", time);
        //printf("rdistance = %.2lfcm\n",rdistance);

        customInsertQueue(rdistance, FLAG_R);
        customInsertQueue(ldistance, FLAG_L);
        if(mr_distance <= 10 && ml_distance <= 10){
            continue;
        }
        printf("mr_distance : %f\n",mr_distance);
        if(mr_distance < 40 && mr_distance > 10){
            rpower = 2;
        }else if(mr_distance < 100 && mr_distance > 10){
            rpower = 1;
        }else{
            rpower = 0;
        }
        if(rpower != 0){
            printf("rpower : %d\n",rpower);
        }else{
            printf("rpower : no change!! \n");
        }
        printf("ml_distance : %f\n",mr_distance);
        if(ml_distance < 40 && ml_distance > 10){
            lpower = 2;
        }else if(ml_distance < 100 && ml_distance > 10){
            lpower = 1;
        }else{
            lpower = 0;
        }
        if(lpower != 0){
            printf("lpower : %d\n",lpower);
        }else{
            printf("lpower : no change!! \n");
        }
        
        usleep(10000);
    }

    if(-1==GPIOUnexport(POUT_R_TRIG)||-1==GPIOUnexport(PIN_R_ECHO)){
        return -1;
    }

	return 0;
}