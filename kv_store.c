#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdatomic.h>

#include "common.h"
#include "ring_buffer.h"

typedef struct bucket {
    key_type k;
    value_type v;
    struct bucket *next;
} bucket;

// a few stuffs below are copied from client

struct bucket *ht; // hashtable

struct ring *ring = NULL;
char *shmem_area = NULL;
int tableSize;
int numThreads;
pthread_t threads[100];

// char* shmem_file;
int shm_size;
char * mem;

// FILE * fd;

void initHashtable() {
    if (tableSize < 0) {
        exit(-1);
    }

    ht = malloc(tableSize * sizeof(struct bucket));

    if (ht == NULL) {
        perror("ht init error");
    }
}

void put(key_type k, value_type v) {
    // fprintf(fd, "putting k = %d v = %d\n", k, v);
    // fflush(fd);

    struct bucket * bkt = &ht[hash_function(k, tableSize)];
    struct bucket ** bptr = &bkt;
    char * null = NULL;
    key_type zeroKey = (key_type) 0;
    // value_type zeroValue = (value_type) 0;

    for(;;) {
        if (*bptr == NULL) {
            bucket * newBucket = (bucket *) malloc(sizeof(struct bucket));
            newBucket->k = k;
            newBucket->v = v;
            if (!atomic_compare_exchange_strong(bptr, &null, (void *)newBucket)) {
                free(newBucket);
            }
            continue;
        } else if ((*bptr)->k == zeroKey) {
            if (atomic_compare_exchange_strong(&((*bptr)->k), &zeroKey, k)) {
                continue;
            }
            // atomic_exchange(&((*bptr)->v), v);
            // break;
        } else if ((*bptr)->k == k) {
            atomic_exchange(&((*bptr)->v), v);
            break;
        } else {
            bptr = &((*bptr)->next);
            continue;
        }
    }
}

value_type get(key_type k) {
    // fprintf(fd, "getting k = %d\n", k);
    // fflush(fd);

    struct bucket * b = &ht[hash_function(k, tableSize)];
    while (b != NULL) {
        if (b->k == k) {
            return b->v;
        }
        b = b->next;
    }

    return 0;
    
}

// sketchy approah by copying over client
int initMem() {
    int myfd = open("shmem_file", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
	if (myfd < 0) {
        perror("open");
    }
    
    struct stat st;
    if (stat("shmem_file", &st) == -1) {
        perror("stat");
    }
    shm_size = st.st_size;

    mem = mmap(NULL, shm_size, PROT_WRITE | PROT_READ, MAP_SHARED, myfd, 0);
	if (mem == NULL) {
        perror("mmap");
    }

    close(myfd);

    ring = (struct ring*) mem;
    shmem_area = mem;
    init_ring(ring);
}

/////// fulfilling request function section below ///////

void copy_bd(struct buffer_descriptor *bd) {
    struct buffer_descriptor *new_bd=(struct buffer_descriptor *)(shmem_area + bd->res_off);
    memcpy(new_bd,bd,sizeof(struct buffer_descriptor));
    new_bd->ready=1;
}

void serveReq(struct buffer_descriptor *bd) {
    // fprintf(fd, "server request type = %d k = %d\n", bd->req_type, bd->k);
    // fflush(fd);
    switch (bd->req_type)
    {
    case PUT:
        put(bd->k, bd->v);
        copy_bd(bd);
        break;

    case GET:
        bd->v = get(bd->k);
        copy_bd(bd);
        break;
    
    default:
        break;
    }
}

void * threadFunc() {
    // fprintf(fd, "threadFunc\n");
    // fflush(fd);

    struct buffer_descriptor bd;
    for(;;) {
        // fprintf(fd, "threadFunc for loop\n");
        // fflush(fd);

        ring_get(ring, &bd);

        // fprintf(fd, "ring get\n");
        // fflush(fd);

        serveReq(&bd);
    }
}

/////// fulfilling request function section above ///////

// the following multithreading functions referring to this
// https://www.geeksforgeeks.org/multithreading-in-c/
void createThreads() {
    for (int i = 0; i < numThreads; i++) {
        if (pthread_create(&threads[i], NULL, &threadFunc, NULL)) {
            perror("pthread_create");
        }
    }
}

void joinThreads() {
    // fprintf(fd, "begin joining thread\n");
    // fflush(fd);

    for (int i = 0; i < numThreads; i++) {
        // fprintf(fd, "joining thread %d\n", i);
        // fflush(fd);

        if (pthread_join(threads[i], NULL)) {
            // fprintf(fd, "thread %d join error\n", i);
            // fflush(fd);
            perror("pthread_join");
        }
    }

    // fprintf(fd, "thread joining done\n");
    // fflush(fd);
}

int main (int argc, char *argv[]) {
    // fd = fopen("results.txt", "w");

    // IMPORTANT: INSERT ARGUMENT PARSER HERE
    char c;
    while((c = getopt(argc, argv, "n:s:")) != -1) {
        switch(c)
        {
            case 's':
                tableSize = atoi(optarg);
                break;
            case 'n':
                numThreads = atoi(optarg);
                break;
            default:
                perror("invalid arg");
                return -1;
        }
    }

    // for (int i = 0; i < argc; i++) {
    //     fprintf(fd, "argument: %s\n", argv[i]);
    //     fflush(fd);
    // }
    
    // fprintf(fd, "calling functions\n");
    // fflush(fd);
    
    // calling functions
    initHashtable();
    initMem();
    createThreads();
    // fprintf(fd, "createThreads\n");
    // fflush(fd);
    joinThreads();

    // fclose(fd);
    
    return 0;
}
