#include "seq_vector.h"
#include <iostream>
#include <cmath>

int main() {
	seq_vector vec (1);
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
}

seq_vector::seq_vector() {
	seq_vector(128);
}

seq_vector::seq_vector(int n) {
	_size = 0;
	_active_levels = 0;
	// calculate max capacity of vector based on number of levels allocated
	_max_capacity = pow(2, NUM_LEVELS + 1) - 2;
	array = (node_t **)calloc(NUM_LEVELS, sizeof(node_t *));
	seq_vector::reserve(n);
}

void seq_vector::pushback(int val) {
	if (_size >= _capacity) {
		seq_vector::reserve(_size + 1);
	}

	int i, j;
	// calculate level to put this element at
	i = floor(log2(_size + 2)) - 1;
	// get element at that level to put this element at
	j = _size - (pow(2, i+1) - 2);

	array[i][j].val = val;
	_size++;
}

int seq_vector::popback() {
	_size--;

	int i, j;
	// calculate level to put this element at
	i = floor(log2(_size + 2)) - 1;
	// get element at that level to put this element at
	j = _size - (pow(2, i+1) - 2);

	return array[i][j].val;
}

void seq_vector::reserve(int n) {
	if (n > _max_capacity) {
		throw std::out_of_range(std::to_string(n) + " elements is too many to fit " +
		"in this vector (max is " + std::to_string(_max_capacity) + ").");
	}

	if (n <= _capacity) {
		return;
	}

	// if vector's capacity was zero, allocate first level before going in to
	// next loop since we need a level to base the capacity of the rest off of
	if (_capacity <= 0) {
		_active_levels = 1;
		_capacity = 2;
		array[0] = (node_t *)calloc(2, sizeof(node_t));
	}

	// allocate arrays in deeper levels until the vector can hold at least n elements
	int curr_capacity = _capacity, prev_level_capacity = pow(2, _active_levels), curr_level_capacity;
	while (curr_capacity < n) {
		// allocate double the number of elements as previous level could hold
		curr_level_capacity = prev_level_capacity * 2;
		prev_level_capacity = curr_level_capacity;
		array[_active_levels++] = (node_t *)calloc(curr_level_capacity, sizeof(node_t));
		curr_capacity += curr_level_capacity;
	}

	_capacity = curr_capacity;
}

int seq_vector::read(int idx) {
	if (idx >= _size) throw std::out_of_range("read() with index " + std::to_string(idx) + " out of bounds.");

	int i, j;
	// calculate level to put this element at
	i = floor(log2(idx + 2)) - 1;
	// get element at that level to put this element at
	j = idx - (pow(2, i+1) - 2);

	return array[i][j].val;
}

void seq_vector::write(int idx, int val) {
	if (idx >= _size) throw std::out_of_range("write() with index " + std::to_string(idx) + " out of bounds.");

	int i, j;
	// calculate level to put this element at
	i = floor(log2(idx + 2)) - 1;
	// get element at that level to put this element at
	j = idx - (pow(2, i+1) - 2);

	array[i][j].val = val;
}

int seq_vector::size() {
	return _size;
}

int seq_vector::capacity() {
	return _capacity;
}
