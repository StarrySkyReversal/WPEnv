#pragma once

#include "DownloadThread.h"

//#define MAX_SIZE 100

typedef struct {
    ThreadManager** items;
    int top;
    int capacity;
    int size;
} Stack;

bool initializeStack(Stack* s, int queueSize);
int stackIsEmpty(Stack* s);
int stackIsFull(Stack* s);
bool stackPush(Stack* s, ThreadManager* item);
bool stackPop(Stack* s, ThreadManager** item);
void freeStack(Stack* s);
