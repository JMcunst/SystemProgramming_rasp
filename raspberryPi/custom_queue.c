#include <stdio.h>
#include <stdlib.h>
#include "custom_queue.h"

double queue_array[QUEUE_MAX];
int rear = -1;
int front = -1;
int main()
{
    int choice;
    while (1)
    {
        printf("1.Insert element to queue n");
        printf("2.Delete element from queue n");
        printf("3.Display all elements of queue n");
        printf("4.Quit n");
        printf("Enter your choice : ");
        scanf("%d", &choice);
        switch (choice)
        {
        case 1:
            insert();
            break;
        case 2:
            delete ();
            break;
        case 3:
            display();
            break;
        case 4:
            exit(1);
        default:
            printf("Wrong choice n");
        }
    }
}
void insertQueue(double distance)
{
    if(distance >= 100.0 ){  //noise happen
        printf("distance is over 100.0 cm. Insert is failed.\n");
        return;
    }
    if (rear == QUEUE_MAX - 1){   //queue is full
        printf("Queue Overflow n");
        refreshQueue();
        front = 0;
        rear = 0;
        queue_array[rear] = distance;
    }else
    {
        if (front == -1)
            front = 0;
        printf("Inset the element in queue : ");
        rear = rear + 1;
        queue_array[rear] = distance;
    }
}

void refreshQueue(){
    memset(queue_array, 0.0, QUEUE_MAX * sizeof(double));
}

/* now deprecated */
void deleteQueue()
{
    if (front == -1 || front > rear)
    {
        printf("Queue Underflow n");
        return;
    }
    else
    {
        printf("Element deleted from queue is : %dn", queue_array[front]);
        front = front + 1;
    }
}
/* now deprecated */
void displayQueue()
{
    int i;
    if (front == -1)
        printf("Queue is empty n");
    else
    {
        printf("Queue is : n");
        for (i = front; i <= rear; i++)
            printf("%d ", queue_array[i]);
        printf("n");
    }
}