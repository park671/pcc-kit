//
// Created by Park Yu on 2024/11/26.
//

#include "stack.h"
#include "mspace.h"

#ifdef __cplusplus
extern "C" {
#endif

static const char *STACK_TAG = "stack";

struct StackNode {
    void *data;
    struct StackNode *next;
};

void push(struct Stack *stack, void *data) {
    struct StackNode *newNode = (struct StackNode *) pccMalloc(stack->spaceTag, sizeof(struct StackNode));
    newNode->data = data;
    newNode->next = NULL;

    if (stack->stackTop != NULL) {
        newNode->next = stack->stackTop;
    }
    stack->stackTop = newNode;
    stack->stackSize++;
}

void *top(struct Stack *stack) {
    if (stack->stackTop == NULL) {
        return NULL;
    }
    return stack->stackTop->data;
}

void *pop(struct Stack *stack) {
    if (stack->stackTop == NULL) {
        return NULL;
    }
    struct StackNode *topNode = stack->stackTop;
    void *data = topNode->data;

    stack->stackTop = topNode->next;

    pccFree(stack->spaceTag, topNode);
    stack->stackSize--;
    return data;
}

int size(struct Stack *stack) {
    return stack->stackSize;
}

void *get(struct Stack *stack, int index) {
    if (index < 0 || index >= stack->stackSize) {
        return NULL;
    }

    struct StackNode *current = stack->stackTop;
    for (int i = 0; i < index; i++) {
        current = current->next;
    }
    return current->data;
}

void resetIterator(struct Stack *stack) {
    stack->it = stack->stackTop;
}

void *iteratorNext(struct Stack *stack) {
    if (stack->it == NULL) {
        return NULL;
    }
    void *result = stack->it->data;
    stack->it = stack->it->next;
    return result;
}

struct Stack *createStack(const char *spaceTag) {
    struct Stack *stack = (struct Stack *) pccMalloc(spaceTag, sizeof(struct Stack));
    stack->spaceTag = spaceTag;
    stack->push = push;
    stack->top = top;
    stack->pop = pop;
    stack->size = size;
    stack->get = get;
    stack->it = NULL;
    stack->resetIterator = resetIterator;
    stack->iteratorNext = iteratorNext;
    stack->stackTop = NULL;
    return stack;
}

#ifdef __cplusplus
}
#endif
