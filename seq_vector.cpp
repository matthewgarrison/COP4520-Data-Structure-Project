// Problems: capacity seems off in some cases (seem correct when set initially,
// but wrong when set as a result of pushback). Check reserve()
// Also check what popback() is returning

#include "seq_vector.h"
#include <iostream>
#include <cmath>

int main() {
	seq_vector vec (25);
	vec.pushback(0);
	vec.pushback(1);
	vec.pushback(2);
	vec.pushback(3);
	vec.pushback(4);
	vec.pushback(5);
	vec.pushback(6);
	vec.pushback(7);
	vec.pushback(8);
	vec.pushback(9);
	vec.pushback(10);
	vec.pushback(11);
	vec.write(0, 56);
	vec.write(10, 234);
	int retval = vec.popback();
	std::cout << retval << std::endl;
	std::cout << vec.size() << std::endl;
	std::cout << vec.capacity() << std::endl;
	std::cout << vec.read(0) << std::endl;
	std::cout << vec.read(1) << std::endl;
	std::cout << vec.read(2) << std::endl;
	std::cout << vec.read(3) << std::endl;
	std::cout << vec.read(4) << std::endl;
	std::cout << vec.read(5) << std::endl;
	std::cout << vec.read(6) << std::endl;
	std::cout << vec.read(7) << std::endl;
	std::cout << vec.read(8) << std::endl;
	std::cout << vec.read(9) << std::endl;
	std::cout << vec.read(10) << std::endl;
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
	int i, j;
	// calculate level to put this element at
	i = floor(log2(_size + 2)) - 1;
	// get element at that level to put this element at
	j = _size - (pow(2, i+1) - 2);

	int ret_val = array[i][j].val;
	_size--;
	return ret_val;
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
