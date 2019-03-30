#include <cstdlib>
#include <cstdio>

// We'll have to decide on a maximum queue size
#define QSize 100000

typedef enum OpType_t {
	PUSH,
	POP
} OpType;

typedef struct Node_t {
	int value;
} Node;

typedef struct WriteDescr_t {
	bool pending;
	size_t pos;
	Node *v_old;
	Node *v_new;
} WriteDescr;

typedef struct Queue_t {
	bool closed;
	void *items[QSize];
	size_t tail;
	size_t head;
} Queue;	

typedef struct Descr_t {
	WriteDescr *opDesc;
	size_t size;
	size_t offset;
	Queue *batch;
	OpType op;
} Descr;

typedef struct Vector_t {
	Node **vdata;
	Descr *descr;
	Queue *batch;
} Vector;

typedef struct th_info_t {
	Vector *root;
	Queue *q;
	size_t offset;
	Queue batch;
} th_info;