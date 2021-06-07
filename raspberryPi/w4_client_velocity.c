#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <math.h>

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
#define POUT_L_TRIG    8
#define PIN_L_ECHO     7

#define FLAG_R 1
#define FLAG_L 2
#define MAXESCOUNT 80
#define MAXEECOUNT 81

double qr_array[QUEUE_MAX] = {0,};
double ql_array[QUEUE_MAX] = {0,};
int rrear = -1;
int rfront = -1;
int lrear = -1;
int lfront = -1;
double mr_distance = 0.0;
double ml_distance = 0.0;

double vnl_distance = 0;
double vpl_distance = 0;
double lspeed = 0;
double vnr_distance = 0;
double vpr_distance = 0;
double rspeed = 0;

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}

void customBubbleSort(double arr[])    // 매개변수로 정렬할 배열과 요소의 개수를 받음
{
    int temp;

    for (int i = 0; i < QUEUE_MAX; i++)    // 요소의 개수만큼 반복
    {
        for (int j = 0; j < QUEUE_MAX - 1; j++)   // 요소의 개수 - 1만큼 반복
        {
            if (arr[j] > arr[j + 1])          // 현재 요소의 값과 다음 요소의 값을 비교하여
            {                                 // 큰 값을
                temp = arr[j];
                arr[j] = arr[j + 1];
                arr[j + 1] = temp;            // 다음 요소로 보냄
            }
        }
    }
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
void customInsertQueue(double qDist, int qLR)
{
    if(qLR == FLAG_R){
        if (rrear == QUEUE_MAX - 1){ // Queue is Full
            //printf("Right Queue is Full \n ");

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
            //printf("Left Queue is Full , ");

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
int main(int argc, char *argv[]){
    int sock;
    struct sockaddr_in serv_addr;
    char msg[6];

    clock_t rstart_t,rend_t;
    clock_t lstart_t,lend_t;
    double rtime;
    double rdistance;
    double ltime;
    double ldistance;

    int rpower;
    int lpower;

    int power;

    if (argc != 3)
    {
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("connect() error");

    if(-1==GPIOUnexport(POUT_R_TRIG)||-1== GPIOUnexport(PIN_R_ECHO)||-1==GPIOUnexport(POUT_L_TRIG)||-1== GPIOUnexport(PIN_L_ECHO)){
        return -1;
    }
    
    if(-1==GPIOExport(POUT_R_TRIG)||-1==GPIOExport(PIN_R_ECHO)||-1==GPIOExport(POUT_L_TRIG)||-1==GPIOExport(PIN_L_ECHO)){
        printf("gpio export err ultra\n");
        exit(0);
    }

    usleep(100000);

    if(-1==GPIODirection(POUT_R_TRIG,OUT) ||-1==GPIODirection(PIN_R_ECHO,IN)||-1==GPIODirection(POUT_L_TRIG,OUT) ||-1==GPIODirection(PIN_L_ECHO,IN)){
        printf("gpio direction err ultra\n");
        exit(0);
    }

    GPIOWrite(POUT_R_TRIG,0);
    GPIOWrite(POUT_L_TRIG,0);
    usleep(10000);

    while(1){
        int es_count = 0;
        // Right Clock 
        if(-1==GPIOWrite(POUT_R_TRIG,1)){
            printf("gpio Right write/trigger err\n");
            exit(0);
        }
        usleep(10);
        GPIOWrite(POUT_R_TRIG,0);
        while(GPIORead(PIN_R_ECHO) == 0){
            es_count++;
            if(es_count > MAXESCOUNT){
                es_count = MAXEECOUNT;
                break;
            }
            rstart_t=clock();
        }
        while(GPIORead(PIN_R_ECHO) == 1){
            if(es_count == MAXEECOUNT){
                es_count = 0;
                break;
            }
            rend_t=clock();
        }
        //printf("Right clock end\n");
        usleep(100);
    
        // Left Clock 
        if(-1==GPIOWrite(POUT_L_TRIG,1)){
            printf("gpio Right write/trigger err\n");
            exit(0);
        }
        usleep(10);
        GPIOWrite(POUT_L_TRIG,0);       
        while(GPIORead(PIN_L_ECHO) == 0){
            es_count++;
            if(es_count > MAXESCOUNT){
                es_count = MAXEECOUNT;
                break;
            }
            lstart_t=clock();
        }
        while(GPIORead(PIN_L_ECHO) == 1){
            if(es_count == MAXEECOUNT){
                es_count = 0;
                break;
            }
            lend_t=clock();
        }
        //printf("Left clock end \n");
        usleep(100);

        //printf("start : %f\n",rstart_t);
        //printf("end : %f\n",end_t);
        //printf("end - start : %f\n",end_t-rstart_t);

        ltime = (double)(lend_t-lstart_t)/CLOCKS_PER_SEC;
        ldistance = ltime/2*34000;
        rtime = (double)(rend_t-rstart_t)/CLOCKS_PER_SEC;
        rdistance = rtime/2*34000;

        customInsertQueue(ldistance, FLAG_L);
        customInsertQueue(rdistance, FLAG_R);
        
        if(mr_distance <= 10 || ml_distance <= 10){
            continue;
        }

        printf("ml_distance : %f, ",ml_distance);
        vnl_distance = vpl_distance;
        vpl_distance = ml_distance;
        lspeed = vpl_distance - vnl_distance;
        if(ml_distance < 40 && ml_distance > 10){
            lpower = 2;
        }else if(ml_distance < 100 && ml_distance > 10){
            lpower = 1;
            if(lspeed > 20 && first_speed != 1){
                lpower = 2;
                printf("Left speed power on.\n");
            }
        }else{
            lpower = 0;
        }
        
        printf("mr_distance : %f\n",mr_distance);
        vnr_distance = vpr_distance;
        vpr_distance = mr_distance;
        rspeed = vpr_distance - vnr_distance;
        if(mr_distance < 40 && mr_distance > 10){
            rpower = 2;
        }else if(mr_distance < 120 && mr_distance > 10){
            rpower = 1;
            if(rspeed > 20 && first_speed != 1){
                rpower = 2;
                printf("Right speed power on.\n");
            }
        }else{
            rpower = 0;
        }
        
        if(lpower != 0){
            printf("lpower : %d, ",lpower);
        }else{
            printf("lpower : no change!!, ");
        }
        if(rpower != 0){
            printf("rpower : %d \n ,",rpower);
        }else{
            printf("rpower : no change!! \n");
        }
        
        if(first_speed == 1){
            first_speed++;
        }
        
        usleep(90000);
        snprintf(msg, sizeof(msg), "m%d%d", lpower, rpower);
        write(sock, msg, sizeof(msg));
        printf("msg = %s\n",msg);
        printf("-----------------------------------\n");
        usleep(200000);
    }

    close(sock);

    if(-1==GPIOUnexport(POUT_R_TRIG)||-1==GPIOUnexport(PIN_R_ECHO)||-1==GPIOUnexport(POUT_L_TRIG)||-1==GPIOUnexport(PIN_L_ECHO)){
        return -1;
    }

	return 0;
    
}