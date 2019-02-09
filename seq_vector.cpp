#include "seq_vector.h"
#include <iostream>

int main() {
	seq_vector vec (20);
	vec.write(0, 5);
	vec.write(1, 6);
	std::cout << vec.read(0) << std::endl;
	std::cout << vec.read(1) << std::endl;
}

seq_vector::seq_vector() {
	seq_vector(128);
}

seq_vector::seq_vector(int n) {
	_size = 0;
	_capacity = n;
	array = (node_t **)malloc(sizeof(node_t) * n);
}

void seq_vector::pushback(int val) {
	if (_size + 1 > _capacity) {
		// Double the capacity.
		seq_vector::reserve(_capacity * 2);
	}
	array[_size] -> val = val;
	_size++;
}

int seq_vector::popback() {
	int ret_val = array[_size-1] -> val;
	_size--;
	return ret_val;
}

void seq_vector::reserve(int n) {
	if (n <= _capacity) return; // Array is already large enough.
	_capacity = n;
	node_t ** newArray = (node_t **)malloc(sizeof(node_t) * _capacity);
	for (int i = 0; i < _size; i++) {
		newArray[i] = array[i];
	}
	// for (int i = _size; i < _capacity; i++) {
		// newArray[i] = 
	// }
	array = newArray;
}

int seq_vector::read(int idx) {
	if (idx >= _size) throw std::out_of_range("read() with index " + std::to_string(idx) + " out of bounds.");
	return array[idx] -> val;
}

void seq_vector::write(int idx, int val) {
	if (idx >= _size) throw std::out_of_range("write() with index " + std::to_string(idx) + " out of bounds.");
	array[idx] -> val = val;
}

int seq_vector::size() {
	return _size;
}

int seq_vector::capacity() {
	return _capacity;
}