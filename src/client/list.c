#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "list.h"


/**
 * Create a new list.
 */
List createList() {
    List L = (List)malloc(sizeof(Node));
    if (L == NULL) {
        perror("Out of memory!");
        exit(EXIT_FAILURE);
    }
    L->Next = NULL;
    return L;
}

/**
 * Check if the list is empty.
 */
bool isEmptyList(List L) {
    return L->Next == NULL;
}

/**
 * Check if the position is the last in the list.
 */
bool isLast(Position P) {
    return P->Next == NULL;
}


/**
 * Delete the first occurrence of an element from the list.
 */
void delete(void* X, List L) {
    Position P = L;
    while (P->Next != NULL && P->Next->Element != X) {
        P = P->Next;
    }
    if (P->Next != NULL) {
        Position Temp = P->Next;
        P->Next = Temp->Next;
        free(Temp);
    }
}


/**
 * Insert an element at a given position in the list.
 */
void insert(void* X, List L, Position P) {
    Position Temp = (Position)malloc(sizeof(Node));
    if (Temp == NULL) {
        perror("Out of memory!");
        exit(EXIT_FAILURE);
    }
    Temp->Element = X;
    Temp->Next = P->Next;
    P->Next = Temp;
}


/**
 * Delete the entire list.
 */
void deleteList(List L) {
    Position P = L->Next;
    Position Temp;
    while (P != NULL) {
        Temp = P->Next;
        free(P);
        P = Temp;
    }
    L->Next = NULL;
}


/**
 * Get the header of the list.
 */
Position headerList(List L) {
    return L;
}


/**
 * Get the first element of the list.
 */
Position firstList(List L) {
    return L->Next;
}


/**
 * Get the next element in the list.
 * If the current position is the last, it returns NULL.
 */
Position advanceList(Position P) {
    return P->Next;
}


/**
 * Get the last element of the list.
 */
void* retrieveElementList(Position P) {
    return P->Element;
}