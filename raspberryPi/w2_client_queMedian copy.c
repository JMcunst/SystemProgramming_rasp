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

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1

#define BUFFER_MAX 45
#define DIRECTION_MAX 45
#define VALUE_MAX 256
#define QUEUE_MAX 15

#define POUT_R_TRIG    23
#define PIN_R_ECHO     24

#define R_FLAG 1
#define FDCOUNT 3
#define FDFULLCOUNT 5

double qr_array[QUEUE_MAX] = {0,};
int rear = -1;
int front = -1;
double m_distance = 0.0;

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
void customInsertQueue(double qDist)
{
    if (rear == QUEUE_MAX - 1)
    { // Queue is Full
        printf("Queue is Full.\n");

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
        m_distance = qr_array[QUEUE_MAX/2];

        //Queue Reset
        memset(qr_array, 0.0, QUEUE_MAX * sizeof(double));
        front = 0;
        rear = 0;
        qr_array[rear] = qDist;
    }
    else
    {
        m_distance = 0;
        if (front == -1)
            front = 0;
        //printf("Insert the element in Queue : %.2lfcm\n ",qDist);
        rear = rear + 1;
        qr_array[rear] = qDist;
    }
}
int main(int argc, char *argv[]){
    int sock;
    struct sockaddr_in serv_addr;
    char msg[6];

    clock_t start_t,end_t;
    double time;
    double distance;

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

    if(-1==GPIOUnexport(POUT_R_TRIG)||-1== GPIOUnexport(PIN_R_ECHO)){
        return -1;
    }
    
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
        
        customInsertQueue(distance);
        if(m_distance <= 10){
            continue;
        }
        printf("m_distance : %f\n",m_distance);
        if(m_distance < 40 && m_distance > 10){
            power = 2;
        }else if(m_distance < 100 && m_distance > 10){
            power = 1;
        }else{
            power = 0;
        }
        if(power != 0){
            printf("power : %d\n",power);
        }else{
            printf("power : no change!! \n");
        }

        snprintf(msg, sizeof(msg), "m%d%d", power, power);
        write(sock, msg, sizeof(msg));
        printf("msg = %s\n",msg);
        usleep(200000);
    }

    close(sock);

    if(-1==GPIOUnexport(POUT_R_TRIG)||-1==GPIOUnexport(PIN_R_ECHO)){
        return -1;
    }

	return 0;
    
}