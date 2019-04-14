#include "parallel_vector.h"

int main(int argc, char **argv)
{
	printf("hello world\n");
	return 0; 
}

// returns the index of the bucket for i (level zero of the array).
int comb_vector::get_bucket(int i) {
	int pos = i + FBS;
	int hiBit = comb_vector::highest_bit(pos);
	return hiBit - comb_vector::highest_bit(FBS);
}
// returns the index within the bucket for i (level one of the array).
int comb_vector::get_idx_within_bucket(int i) {
	int pos = i + FBS;
	int hiBit = comb_vector::highest_bit(pos);
	return pos ^ (1 << hiBit);
}

// returns the index of the highest one bit. eg. highest_bit(8) = 3
int comb_vector::highest_bit(int n) {
	return comb_vector::number_of_trailing_zeros(comb_vector::highest_one_bit(n));
}

// from source code for Java Integer class
int comb_vector::number_of_trailing_zeros(unsigned int i) {
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
int comb_vector::highest_one_bit(unsigned int i) {
	i |= (i >>  1);
	i |= (i >>  2);
	i |= (i >>  4);
	i |= (i >>  8);
	i |= (i >> 16);
	return i - (i >> 1);
}
