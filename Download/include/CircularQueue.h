#pragma once

typedef struct {
    int* data;
    int front;
    int rear;
    int max_size;
} CircularQueue;

void InitCircularQueue(CircularQueue* q, int size);

void freeCircularQueue(CircularQueue* q);
int PeekCircular(CircularQueue* q);
int DequeueCircular(CircularQueue* q);
bool EnqueueCircular(CircularQueue* q, int item);
int GetCircularVal(CircularQueue* circularQueueArray);