#include "framework.h"
#include "DownloadQueue.h"

bool initializeQueue(Queue* q, int queueSize) {
    q->items = (DownloadPart*)malloc(queueSize * sizeof(DownloadPart));
    if (!q->items) return false; // Error allocating memory for items

    q->front = 0;
    q->rear = 0;
    q->size = 0;
    q->capacity = queueSize;

    return true;
}

bool queueIsEmpty(Queue* q) {
    return q->front == q->rear;
}

bool queueIsFull(Queue* q) {
    return (q->rear + 1) % q->capacity == q->front;
}

bool enqueue(Queue* q, DownloadPart item) {
    if (!queueIsFull(q)) {
        q->items[q->rear] = item;
        q->rear = (q->rear + 1) % q->capacity;
        q->size++;
    
        return true;
    }
    else {
        return false;
    }
}
    
bool dequeue(Queue* q, DownloadPart* item) {
    if (!queueIsEmpty(q)) {
        *item = q->items[q->front];
        q->front = (q->front + 1) % q->capacity;
        q->size--;
    
        return true;
    }
    else {
        return false;
    }
}

//bool enqueue(Queue* q, DownloadPart* item) {
//    if (!queueIsFull(q)) {
//        q->items[q->rear] = item;
//        q->rear = (q->rear + 1) % q->capacity;
//        q->size++;
//
//        return true;
//    }
//    else {
//        return false;
//    }
//}
//
//bool dequeue(Queue* q, DownloadPart** item) {
//    if (!queueIsEmpty(q)) {
//        *item = q->items[q->front];
//        q->front = (q->front + 1) % q->capacity;
//        q->size--;
//
//        return true;
//    }
//    else {
//        return false;
//    }
//}

void freeQueue(Queue* q) {
    if (q) {
        if (q->items) {
            //for (int i = 0; i < q->size; i++) {
            //    free((void*)q->items[i].version);
            //    free((void*)q->items[i].url);
            //    free((void*)q->items[i].filename);
            //    free((void*)q->items[i].filepath);
            //}

            free(q->items);
        }
        free(q);
    }
}
