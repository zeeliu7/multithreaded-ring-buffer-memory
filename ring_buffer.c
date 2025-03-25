#include "ring_buffer.h"
#include <string.h>

pthread_spinlock_t rSubLock;
pthread_spinlock_t rGetLock;

int init_ring(struct ring *r) {
	if (r == NULL) {
		return -1;
	}

    // r->c_head = 0;
    // r->c_tail = 0;
    // r->p_head = 0;
    // r->p_tail = 0;

    if (pthread_spin_init(&rSubLock, 0) != 0) { 
        return -1; 
    }
    if (pthread_spin_init(&rGetLock, 0) != 0) { 
        return -1; 
    }

	return 0;
}

void ring_submit(struct ring *r, struct buffer_descriptor *bd) {
    // mutex implementation refer to 
    // https://www.geeksforgeeks.org/mutex-lock-for-linux-thread-synchronization/

    pthread_spin_lock(&rSubLock);

    while (r->p_head - r->c_tail >= RING_SIZE);

    memcpy(&r->buffer[r->p_head % RING_SIZE], bd, sizeof(struct buffer_descriptor));

    
    
    r->p_head++;
    r->p_tail++;

    pthread_spin_unlock(&rSubLock);
}

void ring_get(struct ring *r, struct buffer_descriptor *bd) {

    pthread_spin_lock(&rGetLock);

    
    while (r->p_tail <= r->c_head);

    memcpy(bd, &r->buffer[r->c_head % RING_SIZE], sizeof(struct buffer_descriptor));

    
    r->c_head++;
    r->c_tail++;

    pthread_spin_unlock(&rGetLock);
}