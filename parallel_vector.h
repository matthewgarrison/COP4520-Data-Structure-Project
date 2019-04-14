#include <cstdlib>
#include <cstdio>
#include <atomic>
#include <vector>
#include <array>

// We'll have to decide on a maximum queue size
#define QSize 100000
#define NUM_LEVELS 30
#define FBS 2

class comb_vector {
	private:
		Vector global_vector;
		int size, capacity, max_capacity;
	public:
		comb_vector();
		comb_vector(int n);
		void pushback(int val);
		int popback();
		void reserve(int n);
		int read(int idx);
		void write(int idx, int val);
		int size();
		int capacity();
    void allocate_bucket(int bucketIdx);
    int get_bucket(int i);
    int get_idx_within_bucket(int i);
    int highest_bit(int n);
    int number_of_trailing_zeros(unsigned int i);
    int highest_one_bit(unsigned int i);
};

typedef enum OpType_t {
	PUSH,
	POP
} OpType;

// typedef struct Node_t {
// 	int value;
// } Node;

typedef struct WriteDescr_t {
	bool pending;
	size_t pos;
	int v_old;
	int v_new;
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
	std::array<std::atomic<std::vector<std::atomic_int>*>, NUM_LEVELS> vdata;
	Descr *descr;
	Queue *batch;
} Vector;

typedef struct th_info_t {
	Vector *root;
	Queue *q;
	size_t offset;
	Queue *batch;
} th_info;