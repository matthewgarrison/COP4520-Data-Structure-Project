#ifndef SEQ_VECTOR_H
#define SEQ_VECTOR_H

// The vector node.
typedef struct node {
    int val;
} node_t;

#define NUM_LEVELS 30
#define FBS 2

// The vector class.
class seq_vector {
	private:
		node_t ** array;
		int _size, _capacity, _max_capacity;
	public:
		seq_vector();
		seq_vector(int n);
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



#endif
