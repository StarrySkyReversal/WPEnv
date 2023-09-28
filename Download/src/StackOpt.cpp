#include "framework.h"
#include "StackOpt.h"

bool initializeStack(Stack* s, int stackSize) {
    s->items = (ThreadManager**)malloc(stackSize * sizeof(ThreadManager));
    if (!s->items) return false; // Error allocating memory for items

    s->top = -1;
    s->capacity = stackSize;
    s->size = 0;

    return true;
}

int stackIsEmpty(Stack* s) {
    return s->top == -1;
}

int stackIsFull(Stack* s) {
    return s->top == s->capacity - 1;
}

bool stackPush(Stack* s, ThreadManager* item) {
    if (!stackIsFull(s)) {
        s->items[++(s->top)] = item;
        s->size++;

        return true;
    }
    else {
        return false;
    }
}

bool stackPop(Stack* s, ThreadManager** item) {
    if (!stackIsEmpty(s)) {
        *item = s->items[s->top--];
        s->size--;
        return true;
    }
    else {
        return false;
    }
}

void freeStack(Stack* s) {
    if (s) {
        if (s->items) {
            free(s->items);
        }
    }
}