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
#define QUEUE_MAX 10

#define POUT_R_TRIG    23
#define PIN_R_ECHO     24

#define R_FLAG 1
#define FDCOUNT 3
#define FDFULLCOUNT 5

double vr_drone_speed = 0;	
double vr_now_dist = 0;
double vr_pre_dist = 0;

double qr_array[QUEUE_MAX] = {0,};
double fr_distance;

int rrear = -1;
int rfront = -1;
int isRQFull = 0; // false 

int frdCount = 0;
int frdFullCount = 0;
double frdSum = 0;
double frdValue = 0;

double rmin = 0;
double rmax = 0;
double r_min_to_max = 0;
int rminmaxCount = 0;

int isRFirst = 0;

int power = 0;

void customBubbleSort(double arr[]);
void customMinMax(double qDist);
void customInsertQueue(double qDist);
void customRefreshQueue();

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
void customMinMax(double qDist)
{
    if (rmin > qDist)
    {
        rmin = qDist;
    }
    else if (rmax < qDist)
    {
        rmax = qDist;
    }
    r_min_to_max = rmax - rmin;
    rminmaxCount++;
    printf("--------------------------\n");
    printf("r_min_to_max : %.3lf, count: %d\n", r_min_to_max, rminmaxCount);
}
void customInsertQueue(double qDist)
{
    if (rrear == QUEUE_MAX - 1)
    { //Right Queue is Full
        printf("Right Queue is Full. isRQFull = 1 \n");
        printf("step3\n");
        fr_distance = qr_array[5];

        if (fr_distance < 500){
            power = 2;
        }else if (fr_distance < 300)
        {
            power = 1;
        }else{
            power = 0;
        }
        usleep(50000);
        customRefreshQueue();
        printf("step4\n");
        rfront = 0;
        rrear = 0;
        qr_array[rrear] = qDist;
    }
    else
    {
        if (rfront == -1)
            rfront = 0;
        printf("Inset the element in Right Queue : %.2lfcm\n ",qDist);
        rrear = rrear + 1;
        qr_array[rrear] = qDist;
    }
}

void customRefreshQueue()
{
    memset(qr_array, 0.0, QUEUE_MAX * sizeof(double));
    isRQFull = 0;
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
int main(int argc, char *argv[])
{
    if(-1==GPIOUnexport(POUT_R_TRIG)||-1== GPIOUnexport(PIN_R_ECHO)){
        return -1;
    }
    clock_t rstart_t,rend_t;
    double rdistance;
	double rtime;

    int sock;
    struct sockaddr_in serv_addr;
    char msg[4];

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
    while (1)
    {
        int i=0;
        int j=0;
        if(-1==GPIOWrite(POUT_R_TRIG,1)){
            printf("gpio write/trigger err\n");
            exit(0);
        }
        usleep(100);
        GPIOWrite(POUT_R_TRIG,0);
        printf("--------------------------\n");

        while(GPIORead(PIN_R_ECHO)==0){
            rstart_t=clock();
            if(i++>100){
                break;
            }
        }
        printf("step1\n");
        while(GPIORead(PIN_R_ECHO)==1){
            rend_t=clock();
            if(j++>100){
                break;
            } 
        }
        printf("step2\n");
        printf("end : %.4lf \n start : %.4lf\n", rend_t, rstart_t);

        rtime=(double)fabs(rend_t-rstart_t)/CLOCKS_PER_SEC;
        rdistance=rtime/2*34000;

        if(rdistance > 900){
            rdistance = 900;
        }
        if (rdistance < 0)
        {
            rdistance = 0;
        }

        if (rdistance < 500){
            power = 2;
        }else if (rdistance < 300){
            power = 1;
        }else{
            power = 0;
        }
    
        // customInsertQueue(rdistance);
        printf("step5\n");

        printf("rtime: %.4lf\n",rtime);
        //printf("rdistance: %.2lfcm\n",fr_distance);
        usleep(50000);

        snprintf(msg, sizeof(msg), "m%d%d", power, power);
        write(sock, msg, sizeof(msg));
        printf("msg = %s\n",msg);
        usleep(10000);
        printf("step6\n");
    }
    close(sock);
    //Disable GPIO pins
    
    return (0);
}