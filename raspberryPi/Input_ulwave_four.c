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
#define POUT_L_TRIG      17
#define PIN_L_ECHO      22
#define R_FLAG 1
#define L_FLAG 2
#define FDCOUNT 3
#define FDFULLCOUNT 5


double rdistance;
double fr_distance;
double vr_drone_speed = 0;   
double vr_now_dist = 0;
double vr_pre_dist = 0;

double ldistance;
double fl_distance;
double vl_drone_speed = 0;   
double vl_now_dist = 0;
double vl_pre_dist = 0;

double qr_array[QUEUE_MAX] = {0,};
double ql_array[QUEUE_MAX] = {0,};

int rrear = -1;
int rfront = -1;
int isRQFull = 0; // false 

int lrear = -1;
int lfront = -1;
int isLQFull = 0; // false

int frdCount = 0;
int fldCount = 0;
int frdFullCount = 0;
int fldFullCount = 0;
double frdSum = 0;
double fldSum = 0;
double frdValue = 0;
double fldValue = 0;

double rmin = 0;
double rmax = 0;
double lmin = 0;
double lmax = 0;
double r_min_to_max = 0;
double l_min_to_max = 0;
int rminmaxCount = 0;
int lminmaxCount = 0;

  clock_t rstart_t,rend_t;
   clock_t lstart_t,lend_t;
    double rtime;
   double ltime;

   int isRFirst = 0;
   int isLFirst = 0;

void customInsertQueue(double qDist, int qLR);
void customRefreshQueue(int qLR);
void customBubbleSort(double arr[]);
void customMinMax(double qDist, int qLR);

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
void customMinMax(double qDist, int qLR){
   if(qLR == R_FLAG){
      if(rmin > qDist){
         rmin = qDist;
      }else if(rmax < qDist ){
         rmax = qDist;
      }
      r_min_to_max = rmax - rmin;
      rminmaxCount++;
      printf("--------------------------\n");
      printf("r_min_to_max : %.3lf, count: %d\n",r_min_to_max, rminmaxCount);
   }else{
      if(lmin > qDist){
         lmin = qDist;
      }else if(lmax < qDist ){
         lmax = qDist;
      }
      l_min_to_max = lmax - lmin;
      lminmaxCount++;
      printf("l_min_to_max : %.3lf, count: %d\n",l_min_to_max, lminmaxCount);
   }
}
void customInsertQueue(double qDist, int qLR)
{
   if (qLR == R_FLAG)   // Right Side 
   {
      if (rrear == QUEUE_MAX - 1)
      { //Right Queue is Full
         //printf("Right Queue is Full. isRQFull = 1 \n");
         isRQFull = 1; // true

         rfront = 0;
         rrear = 0;
         qr_array[rrear] = qDist;
      }
      else
      {
         if (rfront == -1)
            rfront = 0;
         //printf("Inset the element in Right Queue : %.2lfcm\n ",qDist);
         rrear = rrear + 1;
         qr_array[rrear] = qDist;
      }
   }else{   // Left Side
      if (lrear == QUEUE_MAX - 1)
      { //Left Queue is Full
         //printf("Left Queue is Full. isLQFull = 1\n");
         isLQFull = 1; // true

         lfront = 0;
         lrear = 0;
         ql_array[lrear] = qDist;
      }
      else
      {
         if (lfront == -1)
            lfront = 0;
         //printf("Inset the element in Left Queue : %.2lfcm\n",qDist);
         lrear = lrear + 1;
         ql_array[lrear] = qDist;
      }
   }
}

void customRefreshQueue(int qLR){
   if(qLR == R_FLAG ){
       memset(qr_array, 0.0, QUEUE_MAX * sizeof(double));
      isRQFull = 0;
   }else{
      memset(ql_array, 0.0, QUEUE_MAX * sizeof(double));
      isLQFull = 0;
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

void *ultra1(){
while(1){
        if(-1==GPIOWrite(POUT_R_TRIG,1)){
            printf("gpio Right write/trigger err\n");
            exit(0);
        }
     
  while(GPIORead(PIN_R_ECHO) == 0){
            rstart_t=clock();
        }
      printf("step1\n");
        while(GPIORead(PIN_R_ECHO) == 1){
            rend_t=clock();
        }
      printf("step2\n");
        rtime = (double)(rend_t-rstart_t)/CLOCKS_PER_SEC;
        GPIOWrite(POUT_R_TRIG,0);
           rdistance = rtime/2*34000;
            customInsertQueue(rdistance, R_FLAG);
      
      usleep(50000);

      if(isRQFull){
         customBubbleSort(qr_array);
         fr_distance = qr_array[5];
         if(isRFirst == 0){
            rmin = fr_distance;
            rmax = fr_distance;
            isRFirst = 1;
         }
         customMinMax(fr_distance, R_FLAG);
         printf("<<<<<<<<fr_distance = %.2lfcm\n",fr_distance);
         printf("vr_drone_speed : %.2lf \n", vr_drone_speed);
         customRefreshQueue(R_FLAG);
      }
      if(fr_distance < 1000.0){
         frdFullCount++;
      }else{
         frdFullCount = 0;
         frdSum = 0;
         frdCount = 0;
      }
      if(frdFullCount >= FDFULLCOUNT){            // Excute 'Collision Avoidance'
         frdCount++;
         frdFullCount++;
         frdSum += fr_distance;
         if(frdCount == FDCOUNT){
            frdValue = frdSum / FDCOUNT; 
            frdCount = 0;
         }
      }else{                        // Don't excute 'Collision Avoidance' - Normal Condition
         //printf("not working Collision! now distance : %.2lfcm\n", fr_distance);
         usleep(50000);
      }
}
      
}

void *ultra2(){
    while(1){
    
      if(-1==GPIOWrite(POUT_L_TRIG,1)){
            printf("gpio Left write/trigger err\n");
            exit(0);
        }
  while(GPIORead(PIN_L_ECHO) == 0){
            lstart_t=clock();
        }
      printf("step3\n");
        //lstart_t=clock();
    
      while(GPIORead(PIN_L_ECHO) == 1){
            lend_t=clock();
        }
      printf("step4\n");

      ltime = (double)(lend_t-lstart_t)/CLOCKS_PER_SEC;
      GPIOWrite(POUT_L_TRIG,0);
      ldistance = ltime/2*34000;
      customInsertQueue(ldistance, L_FLAG);
      usleep(50000);
       
      if(isLQFull){
         customBubbleSort(ql_array);
         fl_distance = ql_array[5];
         if(isLFirst == 0){
            lmin = fl_distance;
            lmax = fl_distance;
            isLFirst = 1;
         }
         customMinMax(fl_distance, L_FLAG);
         printf("<<<<<<<<fl_distance = %.2lfcm\n",fl_distance);
         printf("vl_drone_speed : %.2lf \n", vl_drone_speed);
         customRefreshQueue(L_FLAG);
      }
      if(fl_distance < 1000.0){
         fldFullCount++;
      }else{
         fldFullCount = 0;
      }
      if(fl_distance < 1000.0){            // Excute 'Collision Avoidance'
         fldCount++;
         fldFullCount++;
         fldSum += fl_distance;
         if(fldCount == FDCOUNT){
            fldValue = fldSum / FDCOUNT; 
            fldCount = 0;
         }
      }else{                        // Don't excute 'Collision Avoidance' - Normal Condition
         //printf("not working Collision! now distance : %.2lfcm\n", fl_distance);
         usleep(50000);
      }
    }
   
}

int main(){
   if(-1==GPIOUnexport(POUT_L_TRIG)||-1==GPIOUnexport(POUT_R_TRIG)||-1==GPIOUnexport(PIN_L_ECHO) || GPIOUnexport(PIN_R_ECHO)){
        return -1;
    }
  
      pthread_t p_thread[2];
    int thr_id;
    int status;

    char p1[] = "thread_1";
    char p2[] = "thread_2";




    if(-1==GPIOExport(POUT_R_TRIG)||-1==GPIOExport(PIN_R_ECHO) || -1==GPIOExport(POUT_L_TRIG)||-1==GPIOExport(PIN_L_ECHO)){
        printf("gpio export err ultra\n");
        exit(0);
    }

    usleep(100000);

    if(-1==GPIODirection(POUT_R_TRIG,OUT) ||-1==GPIODirection(PIN_R_ECHO,IN) || -1==GPIODirection(POUT_L_TRIG,OUT)||-1==GPIODirection(PIN_L_ECHO,IN)){
        printf("gpio direction err ultra\n");
        exit(0);
    }

    GPIOWrite(POUT_R_TRIG,0);
    GPIOWrite(POUT_L_TRIG,0);
    usleep(10000);

    thr_id = pthread_create(&p_thread[0], NULL, ultra1, NULL);
    if(thr_id < 0){
        perror("thread create error : ");
        exit(0);
        usleep(1000);
    }
    thr_id = pthread_create(&p_thread[0], NULL, ultra2, NULL);
    if(thr_id < 0){
        perror("thread create error : ");
        exit(0);
        usleep(1000);
    }

}

//sys/class/gpio