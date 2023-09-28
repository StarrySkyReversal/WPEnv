#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include "CircularQueue.h"

void InitCircularQueue(CircularQueue* q, int size) {
    q->data = (int*)malloc(size * sizeof(int));
    q->front = 0;
    q->rear = 0;
    q->max_size = size;
}

bool CircularQueueIsEmpty(CircularQueue* q) {
    return q->front == q->rear;
}

bool CircularQueueIsFull(CircularQueue* q) {
    return (q->rear + 1) % q->max_size == q->front;
}

bool EnqueueCircular(CircularQueue* q, int item) {
    if (CircularQueueIsFull(q)) {
        return false;
    }
    q->data[q->rear] = item;
    q->rear = (q->rear + 1) % q->max_size;
    return true;
}

int DequeueCircular(CircularQueue* q) {
    if (CircularQueueIsEmpty(q)) {
        return -1;
    }
    int item = q->data[q->front];
    q->front = (q->front + 1) % q->max_size;
    return item;
}

int PeekCircular(CircularQueue* q) {
    if (CircularQueueIsEmpty(q)) {
        return -1;
    }
    return q->data[q->front];
}

void freeCircularQueue(CircularQueue* q) {
    free(q->data);
}

int GetCircularVal(CircularQueue* circularQueueArray) {
    int item = PeekCircular(circularQueueArray);
    int deItem = DequeueCircular(circularQueueArray);
    EnqueueCircular(circularQueueArray, deItem);

    return item;
}
