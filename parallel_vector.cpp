#include "parallel_vector.h"
th_info __thread *comb_vector::info;

int main(int argc, char **argv)
{
	printf("hello world\n");
	return 0;
}

write_descr::write_descr() {  }
write_descr::write_descr(int o, int n, int p) {
	pending = true;
	v_old = o;
	v_new = n;
	pos = p;
}

descr::descr(int s, write_descr *wd, op_type ot) {
	size = s;
	offset = -1;
	write_op = wd;
	op = ot;
	// batch = ???
}

comb_vector::comb_vector() {

}

comb_vector::comb_vector(int n) {

}

int comb_vector::popback() {
	return 0;
}

void comb_vector::pushback(int val) {
	bool will_add_to_batch = false, help = false;
	descr *curr_d = nullptr, *new_d = nullptr;
	while(true) {
		// POSSIBLE MEMORY LEAK WITH CURR_D
		curr_d = global_vector->vector_desc.load();

		// Complete any pending operation.
		comb_vector::complete_write(curr_d->write_op);

		// If there's a current or ready Combine operation, this thread will
		// help complete it.
		if(curr_d->batch != nullptr) {
			comb_vector::combine(curr_d, true);
		}

		// Determine which bucket this element will go in.
		int bucket_idx = get_bucket(curr_d->size);
		// If the appropriate bucket doesn't exist, create it.
		if (global_vector->vdata[bucket_idx] == nullptr)
			comb_vector::allocate_bucket(bucket_idx);
		// Create a new Descriptor and WriteDescriptor.
		write_descr *writeop =
			new write_descr(read_unsafe(curr_d->size), val, curr_d->size);
		// writeOp->pending = true;
		// writeOp->v_old = ;
		// writeOp->v_new = val;
		// writeOp->pos = curr_d->size;

		new_d = new descr(curr_d->size + 1, writeop, op_type::PUSH);

		// If our CAS failed (in a previous loop iteration) or this thread
		// has already added items to the queue, then we'll try to add this
		// operation to the queue. (Once we add one operation to the queue,
		//we'll keep doing so until that queue closes.)
		if (will_add_to_batch) {
			//|| (threadInfo.q != null && threadInfo.q == batch.get())) {
			if(add_to_batch(new_d)) {
				return; // The operation was added to the queue
			}

			// We couldn't add it to the queue.
			// [[Not sure why we set newDesc.writeOp to null -
			// we didn't add it to the queue, so where did it go?]]
			free(new_d);
			new_d = new descr(curr_d->size, nullptr, NONE);
			help = true;
			//threadInfo.q = null;
		}

		// Try the normal compare and set.
		if (global_vector->vector_desc.compare_exchange_strong(curr_d, new_d)) {
			if (new_d->batch != nullptr) {
				// AddToBatch set the descriptor's queue, which only happens
				// when we're ready to combine.
				combine(new_d, true);
				if (help) { // [[We're going to help another thread, I guess?]]
					help = false;
					continue;
				}
			}
			break; // We're done.
		} else {
			// The thread adds the operation to the combining queue
			// (in the next loop iteration).
			will_add_to_batch = true;
		}
	}

	complete_write(new_d->write_op);
	comb_vector::info->offset += 1;
	//threadInfo.size = newDesc.size;
}

void comb_vector::reserve(int n) {

}

int comb_vector::read(int idx) {
	return 0;
}

void comb_vector::write(int idx, int val) {

}

int comb_vector::get_size() {
	return 0;
}

int comb_vector::get_capacity() {
	return 0;
}

/////////////////////////////////

bool comb_vector::add_to_batch(descr *d){
	return false;
}

int comb_vector::combine(descr *d, bool dont_need_to_return) {
	return 0;
}

bool comb_vector::inbounds(int idx) {
	return false;
}

void comb_vector::marknode(int idx) {

}

void comb_vector::complete_write(write_descr *writeop) {

}

void comb_vector::allocate_bucket(int bucketIdx) {

}

int comb_vector::read_unsafe(int idx) { // No bounds checking
	return 0;
}

/////////////////////////////////

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
	int temp = comb_vector::highest_one_bit(n);
	return comb_vector::number_of_trailing_zeros(temp);
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
