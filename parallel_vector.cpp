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
	bool will_add_to_batch = false, help_with_combine = false;
	descr *curr_d = nullptr, *new_d = nullptr;
	while(true) {
		// POSSIBLE MEMORY LEAK WITH CURR_D
		curr_d = global_vector->vector_desc.load();

		// Complete any pending operation.
		comb_vector::complete_write(curr_d->write_op);

		// If there's a current or ready Combine operation, this thread will
		// help complete it.
		if(curr_d->batch != nullptr) {
			comb_vector::combine(comb_vector::info, curr_d, true);
		}

		// Determine which bucket this element will go in.
		int bucket_idx = get_bucket(curr_d->size);
		// If the appropriate bucket doesn't exist, create it.
		if (global_vector->vdata[bucket_idx] == nullptr)
			comb_vector::allocate_bucket(bucket_idx);

		// Create a new Descriptor and WriteDescriptor.
		write_descr *writeop =
			new write_descr(read_unsafe(curr_d->size), val, curr_d->size);
		new_d = new descr(curr_d->size + 1, writeop, op_type::PUSH);

		// If our CAS failed (in a previous loop iteration) or this thread
		// has already added items to the queue, then we'll try to add this
		// operation to the queue. (Once we add one push to the queue, we'll
		// keep doing so until that queue closes.)
		if (will_add_to_batch || (comb_vector::info->q != nullptr && 
				comb_vector::info->q == comb_vector::global_vector->batch.load())) {
			if(add_to_batch(new_d)) {
				return; // The operation was added to the queue
			}

			// We couldn't add it to the queue. Therefore, we'll try to help 
			// with the Combine that is happening. If the CAS below succeeds, 
			// we'll do another loop (because help_with_combine is true) and 
			// add val then. If it fails, we'll do another loop and add writeOp 
			// to a new combining queue (because will_add_to_batch is true).
			new_d->size = curr_d->size;
			new_d->write_op = NULL;
			help_with_combine = true;
			comb_vector::info->q = nullptr;
		}

		// Try the normal compare and set.
		if (global_vector->vector_desc.compare_exchange_strong(curr_d, new_d)) {
			if (new_d->batch != nullptr) {
				// AddToBatch set the descriptor's queue, which only happens
				// when we're ready to combine.
				combine(comb_vector::info, new_d, true);
				if (help_with_combine) {
					// We failed the AddToBatch above, so now that we're done 
					// helping with Combine, we're going to do another loop 
					// (because we haven't added writeOp to the vector/combining 
					// queue yet).
					help_with_combine = false;
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
	comb_vector::info->offset = new_d->size;
}

void comb_vector::reserve(int n) {

}

// if the given index is valid, returns the data at that index
// otherwise, returns MARKED
int comb_vector::read(int idx) {
	return check_bounds(idx);
}

// if the given index is valid, attempts to CAS given value with value at given index
// if given index is invalid or CAS fails, returns false; otherwise, returns true
bool comb_vector::write(int idx, int val) {
	int data = check_bounds(idx);

	if (data == MARKED)
		return false;

	int i = get_bucket(idx), j = get_idx_within_bucket(idx);
	return true;
}

// if the given index is valid, returns the data at that index
// otherwise, returns MARKED
int comb_vector::check_bounds(int idx) {
	// read thread-local size
	int size = comb_vector::info->offset;

	// if index is out of bounds when compared to thread-local version of size,
	// check global version of size
	if (idx >= size) {
		size = comb_vector::get_size();
		// now, check again; if index is still out of bounds, it's actually out of bounds
		if (idx >= size) return MARKED;
	}

	// get and return data at the given index
	int i = get_bucket(idx), j = get_idx_within_bucket(idx);
	// read atomically ---
	int data = 0;
	// int data = ((global_vector->vdata[i]).load())[j].load();

	return data;
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

int comb_vector::combine(th_info *info, descr *d, bool dont_need_to_return) {
	return 0;
}

bool comb_vector::inbounds(int idx) {
	return false;
}

void comb_vector::marknode(int idx) {

}

void comb_vector::complete_write(write_descr *writeop) {

}

void comb_vector::allocate_bucket(int bucket_idx) {
	int bucket_size = 1 << (bucket_idx + comb_vector::highest_bit(FBS));
	std::vector<std::atomic_int> *new_bucket = new std::vector<std::atomic_int>();
	std::vector<std::atomic_int> *old_bucket = nullptr;
	if (!(comb_vector::global_vector -> vdata[bucket_idx]).compare_exchange_strong(old_bucket, new_bucket)) {
		// Another thread CASed before us, so free this bucket.
		free(new_bucket);
	}
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
