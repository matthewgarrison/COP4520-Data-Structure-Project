#ifndef SEQ_VECTOR_H
#define SEQ_VECTOR_H

// The vector node.
typedef struct node {
    int val;
} node_t;

// The vector class.
class seq_vector {
	private:
		node_t ** array;
		int _size, _capacity;
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
};



#endif
