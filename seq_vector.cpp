#include "seq_vector.h"
#include <iostream>
#include <cmath>
#include <vector>
#include <string>

int main() {
	std::cout << "Our vector:" << std::endl;
	seq_vector vec (0);
	vec.reserve(200);
	for (int i=0; i<100; i++) {
		vec.pushback(i);
	}

	std::cout << "Size: " + std::to_string(vec.size()) << std::endl;
	std::cout << "Capacity: " + std::to_string(vec.capacity()) << std::endl;

	std::cout << "Element 0: " + std::to_string(vec.read(0)) << std::endl;
	std::cout << "Element 27: " + std::to_string(vec.read(27)) << std::endl;
	std::cout << "Element 13: " + std::to_string(vec.read(13)) << std::endl;
	std::cout << "Element 14: " + std::to_string(vec.read(14)) << std::endl;
	std::cout << "Element 99: " + std::to_string(vec.read(99)) << std::endl;

	int val = vec.popback();
	std::cout << "Pop return value: " + std::to_string(val) << std::endl;
	std::cout << "Size after popping: " + std::to_string(vec.size()) << std::endl;
	std::cout << "Capacity after popping: " + std::to_string(vec.capacity()) << std::endl;

	vec.write(84, 12345);
	std::cout << "Value of index 84 after writing 12345: " + std::to_string(vec.read(84)) << std::endl;

	std::cout << "----------------------------------------------------" << std::endl << "std::vector:" << std::endl;

	std::vector<int> vec2 ;
	vec2.reserve(200);
	vec2[50] = 646178;
	std::cout << "Element 50: " + std::to_string(vec2[50]) << std::endl;
	std::cout << "Size: " + std::to_string(vec2.size()) << std::endl;

	std::cout << "100 pushes" << std::endl;
	for (int i=0; i<100; i++) {
		vec2.push_back(i);
	}

	std::cout << "Size: " + std::to_string(vec2.size()) << std::endl;
	std::cout << "Capacity: " + std::to_string(vec2.capacity()) << std::endl;

	std::cout << "Element 0: " + std::to_string(vec2[0]) << std::endl;
	std::cout << "Element 27: " + std::to_string(vec2[27]) << std::endl;
	std::cout << "Element 13: " + std::to_string(vec2[13]) << std::endl;
	std::cout << "Element 14: " + std::to_string(vec2[14]) << std::endl;
	std::cout << "Element 99: " + std::to_string(vec2[99]) << std::endl;

	val = vec2.back(); vec2.pop_back();
	std::cout << "Pop return value: " + std::to_string(val) << std::endl;
	std::cout << "Size after popping: " + std::to_string(vec2.size()) << std::endl;
	std::cout << "Capacity after popping: " + std::to_string(vec2.capacity()) << std::endl;

	vec2[84] = 12345;
	std::cout << "Value of index 84 after writing 12345: " + std::to_string(vec2[84]) << std::endl;
}

// default constructor: creates a vector with capacity 128
seq_vector::seq_vector() {
	seq_vector(128);
}

// creates a vector with an initial capacity of n
seq_vector::seq_vector(int n) {
	_size = 0;
	_capacity = 0;
	// calculate max capacity of vector based on number of levels allocated
	_max_capacity = pow(2, NUM_LEVELS + 1) - 2;
	array = (node_t **)calloc(NUM_LEVELS, sizeof(node_t *));
	seq_vector::reserve(n);
}

// adds val to the end of the vector; increments size
void seq_vector::pushback(int val) {
	// increase capacity if inserting this element would put vector size past capacity
	if (_size >= _capacity) {
		seq_vector::reserve(_size + 1);
	}

	array[get_bucket(_size)][get_idx_within_bucket(_size)].val = val;
	_size++;
}

// removes and returns the last element in the vector; decrements size
int seq_vector::popback() {
	_size--;

	return array[get_bucket(_size)][get_idx_within_bucket(_size)].val;
}

// increases the capacity of the vector to be able to hold n elements
void seq_vector::reserve(int n) {
	if (n > _max_capacity) {
		throw std::out_of_range(std::to_string(n) + " elements is too many to fit " +
		"in this vector (max is " + std::to_string(_max_capacity) + ").");
	}

	// don't do anything if vector can already hold n elements
	if (n <= _capacity) {
		return;
	}

	// if vector's capacity was zero, allocate first level before going in to
	// next loop since we need a level to base the capacity of the rest off of
	if (_capacity <= 0) {
		_capacity = 2;
		array[0] = (node_t *)calloc(2, sizeof(node_t));
	}

	// the index of the largest in-use bucket.
	int i = get_bucket(_size - 1);
	if (i < 0) i = 0;

	// add new buckets until we have enough buckets for n elements.
	while (i < get_bucket(n - 1)) {
		i++;
		seq_vector::allocate_bucket(i);
		_capacity = (2 * _capacity) + 2;
	}
}

// returns the value at the given index (if the index is valid)
int seq_vector::read(int idx) {
	if (idx >= _size) throw std::out_of_range("read() with index " + std::to_string(idx) + " out of bounds.");

	return array[get_bucket(idx)][get_idx_within_bucket(idx)].val;
}

// writes the given value to the given index (if the index is valid)
void seq_vector::write(int idx, int val) {
	if (idx >= _size) throw std::out_of_range("write() with index " + std::to_string(idx) + " out of bounds.");

	array[get_bucket(idx)][get_idx_within_bucket(idx)].val = val;
}

// returns the current size of the vector
int seq_vector::size() {
	return _size;
}

// returns the current capacity of the vector
int seq_vector::capacity() {
	return _capacity;
}

// allocate a bucket at the given index of the two-level array
void seq_vector::allocate_bucket(int bucketIdx) {
	int bucketSize = 1 << (bucketIdx + seq_vector::highest_bit(FBS));
	array[bucketIdx] = (node_t *)calloc(bucketSize, sizeof(node_t));
}

// returns the index of the bucket for i (level zero of the array).
int seq_vector::get_bucket(int i) {
	int pos = i + FBS;
	int hiBit = seq_vector::highest_bit(pos);
	return hiBit - seq_vector::highest_bit(FBS);
}
// returns the index within the bucket for i (level one of the array).
int seq_vector::get_idx_within_bucket(int i) {
	int pos = i + FBS;
	int hiBit = seq_vector::highest_bit(pos);
	return pos ^ (1 << hiBit);
}

// returns the index of the highest one bit. eg. highest_bit(8) = 3
int seq_vector::highest_bit(int n) {
	return seq_vector::number_of_trailing_zeros(seq_vector::highest_one_bit(n));
}

// from source code for Java Integer class
int seq_vector::number_of_trailing_zeros(unsigned int i) {
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
int seq_vector::highest_one_bit(unsigned int i) {
	i |= (i >>  1);
	i |= (i >>  2);
	i |= (i >>  4);
	i |= (i >>  8);
	i |= (i >> 16);
	return i - (i >> 1);
}
