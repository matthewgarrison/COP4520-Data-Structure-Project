#include "parallel_vector.h"

// define static variables
th_info __thread *comb_vector::info;
write_descr *comb_vector::EMPTY_SLOT;
write_descr *comb_vector::FINISHED_SLOT;

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

Queue::Queue(write_descr *first_item) {
	tail = 1;
	head = 0;
	closed = false;

	items[0].store(first_item);
	for (int i = 1; i < QSize; i++) {
		items[i].store(comb_vector::EMPTY_SLOT);
	}
}

comb_vector::comb_vector() {

}

comb_vector::comb_vector(int n) {

}

int comb_vector::popback() {
	descr *curr_d = nullptr, *new_d = nullptr;
	int elem = MARKED;
	while (true) {
		curr_d = global_vector->vector_desc.load();

		// Complete any pending operation.
		comb_vector::complete_write(curr_d->write_op);

		// If there's a current or ready Combine operation, this thread will
		// help complete it.
		if(curr_d->batch != nullptr) {
			comb_vector::combine(comb_vector::info, curr_d, true);
		}

		// There's nothing to pop.
		if (curr_d->size == 0 && comb_vector::global_vector->batch.load() == nullptr)
			return MARKED;

		if (curr_d->size == 0) {
			// The vector is empty, but there's stuff in the combining queue,
			// so we keep going.
			elem = MARKED;
		} else {
			elem = read_unsafe(curr_d->size - 1);
		}

		// Create a new Descriptor.
		new_d = new descr(curr_d->size - 1, nullptr, op_type::POP);
		new_d->offset = curr_d->size; // The size of the vector, without this pop.
		// This signals that the Combine operation should start.
		new_d->batch = comb_vector::global_vector->batch.load();

		if (global_vector->vector_desc.compare_exchange_strong(curr_d, new_d)) {
			if (new_d->batch != nullptr &&
					new_d->batch == comb_vector::global_vector->batch.load()) {
				// We need to execute any pending pushes before we can pop. Then we'll return
				// the last element added to the vector by Combine.
				new_d->batch->closed = true;
				elem = comb_vector::combine(comb_vector::info, new_d, false);
			} else {
				// Mark the node as logically deleted.
				comb_vector::marknode(curr_d->size - 1);
			}
			break;
		}
	}

	comb_vector::info->offset = new_d->size;
	return elem;
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

// reserves space in the vector for n elements
void comb_vector::reserve(int n) {
	// the index of the largest in-use bucket.
	int i = get_bucket(comb_vector::global_vector->vector_desc.load()->size - 1);
	if (i < 0) i = 0;

	// add new buckets until we have enough buckets for n elements.
	while (i < get_bucket(n - 1)) {
		i++;
		allocate_bucket(i);
	}
}

// if the given index is valid, returns the data at that index
// otherwise, returns MARKED
int comb_vector::read(int idx) {
	// read thread-local size
	int size = comb_vector::info->offset;

	// if index is out of bounds when compared to thread-local version of size,
	// check global version of size
	if (idx >= size) {
		size = comb_vector::global_vector->vector_desc.load()->size;
		// now, check again; if index is still out of bounds, it's actually out of bounds
		if (idx >= size) return MARKED;
	}

	// get and return data at the given index
	int i = get_bucket(idx), j = get_idx_within_bucket(idx);
	return (*((global_vector->vdata[i]).load()))[j].load();
}

// attempts to write the given value at the node with the given index
bool comb_vector::write(int idx, int val) {
	// call write helper with flag of 0 to tell it to only attempt CAS once
	return write_helper(idx, val, 0);
}

// returns true if the given value is successfully written at the node with the given index,
// returns false otherwise. be_persistent tells us whether we should keep attempting CAS
// until it suceeds or if we can just stop after 1 CAS and return its result
bool comb_vector::write_helper(int idx, int val, int be_persistent) {
	int data = read(idx);

	// if read() returned MARKED, that means idx is invalid
	if (data == MARKED)
		return false;

	// attempt to CAS
	int i = get_bucket(idx), j = get_idx_within_bucket(idx);
	bool succ = (*((global_vector->vdata[i]).load()))[j].compare_exchange_strong(data, val);

	// if CAS succeeds or we're quitters, return here
	if (succ || !be_persistent) {
		return succ;
	}

	// otherwise, keep trying until we succeed
	do {
		data = (*((global_vector->vdata[i]).load()))[j].load();
	} while (!((*((global_vector->vdata[i]).load()))[j].compare_exchange_strong(data, val)));

	return true;
}

// returns the current size and, helps complete active Combine operation (if there is one)
int comb_vector::get_size() {
	// If there's a current or ready Combine operation, this thread will help complete it.
	descr *curr_d = global_vector->vector_desc.load();
	if(curr_d->batch != nullptr) {
		comb_vector::combine(comb_vector::info, curr_d, true);
	}

	// get and return size from global vector's descriptor
	return comb_vector::global_vector->vector_desc.load()->size;
}

int comb_vector::get_capacity() {
	return 0;
}

/////////////////////////////////

bool comb_vector::add_to_batch(descr *d) {
	Queue *queue = comb_vector::global_vector->batch.load();

	// Check if the vector has a combining queue already. If not, we'll make one.
	if (queue == nullptr) {
		Queue *newQ = new Queue(d->write_op);
		Queue *tmp = nullptr;
		if (comb_vector::global_vector->batch.compare_exchange_strong(tmp, newQ)) {
			comb_vector::info->q = newQ;
			return true;
		} else {
			free(newQ);
		}
	}

	queue = comb_vector::global_vector->batch.load(); // In case a different thread CASed before us.
	if (queue == nullptr || queue->closed) {
		// We don't add descr, because the queue is closed or non-existent.
		d->batch = queue;
		return false;
	}

	int ticket = queue->tail.fetch_add(1); // Where we'll insert into the queue.
	if (ticket >= QSize) {
		// The queue is full, so close it and return a failure.
		queue->closed = true;
		d->batch = queue;
		return false;
	}

	write_descr *tmp = comb_vector::EMPTY_SLOT;
	if (!queue->items[ticket].compare_exchange_strong(tmp, d->write_op)) { // Add it to the queue.
		return false; // We failed because of an interfering Combine operation.
	}

	// We successfully added the operation to the queue. Update our thread-local queue's value.
	comb_vector::info->q = queue;
	return true;
}

int comb_vector::combine(th_info *info, descr *d, bool dont_need_to_return) {
	return 0;
}

// marks the node at the given index
void comb_vector::marknode(int idx) {
	// call write helper with flag of 1 to tell it to keep attempting CAS until it succeeds
	write_helper(idx, MARKED, 1);
}

// attempts to complete pushback operation using CAS
void comb_vector::complete_write(write_descr *writeop) {
	// if the given write op is still pending, try to complete it
	if (writeop != nullptr && writeop->pending) {
		int i = get_bucket(writeop->pos), j = get_idx_within_bucket(writeop->pos);
		(*((global_vector->vdata[i]).load()))[j].compare_exchange_strong(writeop->v_old, writeop->v_new);
	}
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

// reads value at given with no bounds checking, pls don't give bad indices
int comb_vector::read_unsafe(int idx) {
	// get and return data at the given index
	int i = get_bucket(idx), j = get_idx_within_bucket(idx);
	return (*((global_vector->vdata[i]).load()))[j].load();
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
