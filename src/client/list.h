#ifndef List_h
#define List_h

#include <stdbool.h>

typedef struct Node *PtrToNode;
typedef PtrToNode List;
typedef PtrToNode Position;

// Node structure
typedef struct Node{
    void* Element;
    Position Next;
} Node;


// List Operations
List createList();
bool isEmptyList( List L );
bool isLast( Position P );
void delete( void* X, List L );
void insert( void* X, List L, Position P );
void deleteList( List L );
Position headerList( List L );
Position firstList( List L );
Position advanceList( Position P );
void* retrieveElementList( Position P );


#endif /* List_h */