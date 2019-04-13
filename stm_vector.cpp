#include <cstdlib>
#include <chrono>
#include <iostream>
#include <signal.h>
#include <pthread.h>
#include <api/api.hpp>
#include <common/platform.hpp>
#include <common/locks.hpp>
#include "bmconfig.hpp"
#include "../include/api/api.hpp"
#include "stm_vector.h"
#include <cmath>
using namespace std;

// constant across all tests
const int NUM_TRANSACTIONS = 500000;
const int NUM_OP_LOOPS = NUM_TRANSACTIONS / 100;

// vary these for performance tests
const int THREAD_COUNT = 4;
const int TRANSACTION_SIZE = 1;
const int PERCENT_PUSHBACK = 50;
const int PERCENT_POPBACK = 50;
const int PERCENT_READ = 0;
const int PERCENT_WRITE = 0;

Config::Config() :
    bmname(""),
    duration(1),
    execute(0),
    threads(THREAD_COUNT),
    nops_after_tx(0),
    elements(256),
    lookpct(34),
    inspct(66),
    sets(1),
    ops(TRANSACTION_SIZE),
    time(0),
    running(true),
    txcount(0)
{
}

Config CFG TM_ALIGN(64);

void* run_thread(void* arg) {
    // each thread must be initialized before running transactions
    TM_THREAD_INIT();

    // cast argument as a pointer to shared STM vector object
    stm_vector *vec = reinterpret_cast<stm_vector*>(arg);

    // execute NUM_TRANSACTIONS total operations on the vector
    int i, j;
    for (i=0; i<NUM_OP_LOOPS; i++) {
	    // execute pushback operations
	    for (j=0; j<PERCENT_PUSHBACK; j++)
		    vec->pushback(j);
	    // execute read operations
	    for (j=0; j<PERCENT_READ; j++)
		    vec->read(j);
	    // execute write operations
	    for (j=0; j<PERCENT_WRITE; j++)
		    vec->write(j, 0);
	    // execute popback operations
	    for (j=0; j<PERCENT_POPBACK; j++)
		    vec->popback();
    }

    // shutdown thread when done
    TM_THREAD_SHUTDOWN();
}

int main(int argc, char** argv) {
    TM_SYS_INIT();

    // get start time of execution
    auto start = chrono::high_resolution_clock::now();
    // original thread must be initalized also
    TM_THREAD_INIT();

    // create and initialize shared vector
    stm_vector vec;
    vec.initialize(0);

    void* args[256];
    pthread_t tid[256];

    // set up configuration structs for the threads we'll create
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

    // actually create the threads
    for (uint32_t j = 1; j < CFG.threads; j++)
        pthread_create(&tid[j], &attr, &run_thread, &vec);

    // all of the other threads should be queued up, waiting to run the
    // benchmark, but they can't until this thread starts the benchmark
    // too...
    run_thread(&vec);

    // everyone should be done.  Join all threads so we don't leave anything
    // hanging around
    for (uint32_t k = 1; k < CFG.threads; k++)
        pthread_join(tid[k], NULL);

    // get end time of exection
    auto end = chrono::high_resolution_clock::now();
    // calculate elapsed execution time
    chrono::duration<double> elapsed = end - start;

    // print results
    cout << "Number of threads: " << THREAD_COUNT << "\n";
    cout << "Transaction size: " << TRANSACTION_SIZE << "\n";
    cout << "Op ratio: " << PERCENT_PUSHBACK << "\% pushback, " << PERCENT_POPBACK << "\% popback, " 
	    << PERCENT_READ << "\% read, " << PERCENT_WRITE << "\% write\n";
    cout << "Total elapsed time: " << elapsed.count() << " seconds\n";

    // And call sys shutdown stuff
    TM_SYS_SHUTDOWN();

    return 0;
}

// default initialization: creates a vector with capacity 128
void stm_vector::initialize() {
	stm_vector::initialize(128);
}

// creates a vector with an initial capacity of n
void stm_vector::initialize(int n) {
	int max_capacity, capacity, size;
	TM_BEGIN(atomic) {
		// calculate max capacity of vector and initalize size and capacity to zero
		max_capacity = pow(2, NUM_LEVELS + 1) - 2;
		capacity = 0;
		size = 0;
		TM_WRITE(_size, size);
		TM_WRITE(_capacity, capacity);
		TM_WRITE(_max_capacity, max_capacity);
		// allocate memory for the first level of the array that represents the vector
		TM_WRITE(array, (node_t **)TM_ALLOC(NUM_LEVELS * sizeof(node_t *)));
	}
	TM_END;
	// reserve space in the vector for n elements
	stm_vector::reserve(n);
}

// adds val to the end of the vector; increments size
void stm_vector::pushback(int val) {
	int size, capacity, reserve_space;
	TM_BEGIN(atomic) {
		// read current values of size and capacity
		size = TM_READ(_size);
		capacity = TM_READ(_capacity);
		// if we don't need to increase capacity, write val to end of vector and update size
		if (size < capacity) {
			TM_WRITE(array[get_bucket(size)][get_idx_within_bucket(size)].val, val);
			TM_WRITE(_size, size+1);
			reserve_space = 0;
		}
		// otherwise, we'll need to reserve space before completing operation
		else {
			reserve_space = 1;
		}
	}
	TM_END;

	// if we already completed the operation, simply return
	if (!reserve_space)
		return;

	// otherwise, reserve more space and complete operation
	stm_vector::reserve(size+1);
	TM_BEGIN(atomic) {
		TM_WRITE(array[get_bucket(size)][get_idx_within_bucket(size)].val, val);
		TM_WRITE(_size, size+1);
	}
	TM_END;
}

// removes and returns the last element in the vector; decrements size
int stm_vector::popback() {
	int size, retval;
	TM_BEGIN(atomic) {
		// get current value of size and decrement
		size = TM_READ(_size) - 1;
		// store value at end of vector and update size
		retval = TM_READ(array[get_bucket(size)][get_idx_within_bucket(size)].val);
		TM_WRITE(_size, size);
	}
	TM_END;

	return retval;
}

// increases the capacity of the vector to be able to hold n elements
void stm_vector::reserve(int n) {
	int max_capacity, capacity, size, i, bucketSize;

	TM_BEGIN(atomic) {
		// get current values of max capacity, capacity, and size
		max_capacity = TM_READ(_max_capacity);
		capacity = TM_READ(_capacity);
		size = TM_READ(_size);

		// check to make sure vector could have capacity for n elements
		if (n > max_capacity) {
			throw std::out_of_range(std::to_string(n) + " elements is too many to fit " +
			"in this vector (max is " + std::to_string(max_capacity) + ").");
		}

		// don't do anything if vector can already hold n elements
		if (n <= capacity && n > 0) {
			return;
		}

		// if vector's capacity was zero, allocate first level before going in to
		// next loop since we need a level to base the capacity of the rest off of
		if (capacity <= 0) {
			capacity = FBS;
			TM_WRITE(array[0], (node_t *)TM_ALLOC(FBS * sizeof(node_t)));
		}

		// the index of the largest in-use bucket
		i = get_bucket(size-1);
		if (i < 0) i = 0;

		// add new buckets until we have enough buckets for n elements.
		while (i < get_bucket(n-1)) {
			i++;
			bucketSize = 1 << (i + stm_vector::highest_bit(FBS));
			TM_WRITE(array[i], (node_t *)TM_ALLOC(bucketSize * sizeof(node_t)));
			capacity = (2*capacity) + 2;
		}
		TM_WRITE(_capacity, capacity);
	}
	TM_END;
}

// returns the value at the given index (if the index is valid)
int stm_vector::read(int idx) {
	int size, retval;
	// calculate indices within two-level array
	int i = get_bucket(idx), j = get_idx_within_bucket(idx);

	TM_BEGIN(atomic) {
		// get current size of value
		size = TM_READ(_size);
		// check that given index is within bounds
		if (idx >= size) throw std::out_of_range("read() with index " + std::to_string(idx) + " out of bounds.");
		// get value at the given index
		retval = TM_READ(array[i][j].val);
	}
	TM_END;

	return retval;
}

// writes the given value to the given index (if the index is valid)
void stm_vector::write(int idx, int val) {
	int size;
	// calculate indices within two-level array
	int i = get_bucket(idx), j = get_idx_within_bucket(idx);

	TM_BEGIN(atomic) {
		// get current value of size
		size = TM_READ(_size);
		// check that given index is within bounds 
		if (idx >= size) throw std::out_of_range("write() with index " + std::to_string(idx) + " out of bounds.");
		// write given value at given index
		TM_WRITE(array[i][j].val, val);
	}
	TM_END;
}

// returns the current size of the vector
int stm_vector::size() {
	int retval;

	TM_BEGIN(atomic) {
		// get current size of vector
		retval = TM_READ(_size);
	}
	TM_END;

	return retval;
}

// returns the current capacity of the vector
int stm_vector::capacity() {
	int retval;

	TM_BEGIN(atomic) {
		// get current capacity of vector
		retval = TM_READ(_capacity);
	}
	TM_END;

	return retval;
}

// allocate a bucket at the given index of the two-level array
void stm_vector::allocate_bucket(int bucketIdx) {
	// calculate size of bucket for given index
	int bucketSize = 1 << (bucketIdx + stm_vector::highest_bit(FBS));

	TM_BEGIN(atomic) {
		// allocate bucket at the given index
		TM_WRITE(array[bucketIdx], (node_t *)TM_ALLOC(bucketSize * sizeof(node_t)));
	}
	TM_END;
}

// returns the index of the bucket for i (level zero of the array).
int stm_vector::get_bucket(int i) {
	int pos = i + FBS;
	int hiBit = stm_vector::highest_bit(pos);
	return hiBit - stm_vector::highest_bit(FBS);
}

// returns the index within the bucket for i (level one of the array).
int stm_vector::get_idx_within_bucket(int i) {
	int pos = i + FBS;
	int hiBit = stm_vector::highest_bit(pos);
	return pos ^ (1 << hiBit);
}

// returns the index of the highest one bit. eg. highest_bit(8) = 3
int stm_vector::highest_bit(int n) {
	return stm_vector::number_of_trailing_zeros(stm_vector::highest_one_bit(n));
}

// from source code for Java Integer class
int stm_vector::number_of_trailing_zeros(unsigned int i) {
	int y;
	if (i == 0) return 32;
	int n = 31;
	y = i <<16; if (y != 0) { n = n -16; i = y; }
	y = i << 8; if (y != 0) { n = n - 8; i = y; }
	y = i << 4; if (y != 0) { n = n - 4; i = y; }
	y = i << 2; if (y != 0) { n = n - 2; i = y; }
	return n - ((i << 1) >> 31);
}

// from source code for Java Integer class
int stm_vector::highest_one_bit(unsigned int i) {
	i |= (i >>  1);
	i |= (i >>  2);
	i |= (i >>  4);
	i |= (i >>  8);
	i |= (i >> 16);
	return i - (i >> 1);
}
