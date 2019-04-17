#ifndef PARA_VECTOR_H
#define PARA_VECTOR_H

#include <cstdlib>
#include <cstdio>
#include <atomic>
#include <vector>
#include <array>
#include <climits>

// We'll have to decide on a maximum queue size
#define QSize 100000
#define NUM_LEVELS 30
#define FBS 2
#define MARKED INT_MIN

enum op_type {
	PUSH,
	POP,
	NONE
};

class write_descr {
	public:
		bool pending;
		size_t pos;
		int v_old;
		int v_new;
		write_descr(int o, int n, int p);
		write_descr();
};

class Queue {
	public:
		bool closed;
		std::array<std::atomic<write_descr *>, QSize> items;
		std::atomic<size_t> tail;
		std::atomic<uint64_t> head; // hindex is first 32 bits, hcount is second 32 bits
		Queue();
		Queue(write_descr* first_item);
};

class descr {
	public:
		write_descr *write_op;
		size_t size;
		size_t offset;
		Queue *batch;
		op_type op;
		descr(int s, write_descr *wd, op_type ot);
		descr();
};

class vector_vars {
	public:
		std::array<std::atomic<std::vector<std::atomic_int>*>, NUM_LEVELS> vdata;
		std::atomic<descr *> vector_desc;
		std::atomic<Queue *> batch;
		vector_vars();
};

// Still not entirely sure how thread locals work, so change this if needed
typedef struct th_info_t {
	//vector_vars root;
	Queue *q;
	size_t offset;
	Queue *batch;
} th_info;

class comb_vector {
	private:
		vector_vars *global_vector;
		int size, capacity, max_capacity;
		static __thread th_info *info;

		bool add_to_batch(descr *d);
		int combine(th_info *info, descr *d, bool dont_need_to_return);
		void marknode(int idx);
		void complete_write(write_descr *writeop);
		void allocate_bucket(int bucket_idx);
		int read_unsafe(int idx);
		bool write_helper(int idx, int val, int be_persistent);

		int get_bucket(int i);
		int get_idx_within_bucket(int i);
		int highest_bit(int n);
		int number_of_trailing_zeros(unsigned int i);
		int highest_one_bit(unsigned int i);
		uint32_t get_first_32_bits(uint64_t n);
		uint32_t get_second_32_bits(uint64_t n);
		uint64_t combine_into_64_bits(uint32_t a, uint32_t b);
	public:
		static write_descr *EMPTY_SLOT, *FINISHED_SLOT;

		comb_vector();
		comb_vector(int n);
		void pushback(int val);
		int popback();
		void reserve(int n);
		int read(int idx);
		bool write(int idx, int val);
		int get_size();
		int get_capacity();
};

#endif
