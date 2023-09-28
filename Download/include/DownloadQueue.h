#pragma once

#include "DownloadThread.h"

//#define QUEUE_SIZE 128

typedef struct {
    DownloadPart** items;
    int front, rear, size, capacity;
} Queue;

bool initializeQueue(Queue* q, int queueSize);

bool enqueue(Queue* q, DownloadPart* item);

bool dequeue(Queue* q, DownloadPart** item);

bool queueIsEmpty(Queue* q);

bool queueIsFull(Queue* q);

void freeQueue(Queue* q);
