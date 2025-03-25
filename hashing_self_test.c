#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "ring_buffer.h"
#include "kv_store.h"

// a few stuffs below are copied from client

struct hashtable *ht;

int tableSize = 10;


void initHashtable() {
    ht = (hashtable*) malloc(sizeof(hashtable));
    if (ht == NULL) {
        exit(-1);
    }

    ht->bucketArray = (bucket**) malloc(tableSize * sizeof(bucket*));

    ht->tableSize = tableSize;
    for (int i = 0; i < tableSize; i++) {
        printf("init buc %d\n",i);
        // printf("0\n");
        ht->bucketArray[i] = (bucket*) malloc(sizeof(bucket));
        // printf("1\n");
        if (ht->bucketArray[i] == NULL) {
            exit(-1);
        }
        // printf("2\n");
        ht->bucketArray[i]->index = i;
        // printf("3\n");
        ht->bucketArray[i]->bucketSize = 10;
        // printf("4\n");
        ht->bucketArray[i]->numNodes = 0;
        // printf("5\n");
        ht->bucketArray[i]->keyArray = (key_type*) malloc(10 * sizeof(key_type));
        // printf("6\n");
        ht->bucketArray[i]->valueArray = (value_type*) malloc(10 * sizeof(value_type));
        // printf("7\n");
    }
    printf("init done\n");
}

// I put "basic" over put/get function since I'm unsure whether they qualifies as the put/get in header
void basic_put(key_type k, value_type v) {

    printf("put 1\n");

    struct bucket *thisBucket = ht->bucketArray[hash_function(k, tableSize)];


    for (int i = 0; i < thisBucket->bucketSize; i++) {
        printf("put 2\n");
        if (thisBucket->keyArray[i] == k) {
            if (thisBucket->valueArray[i] != v) {
                thisBucket->valueArray[i] = v;
            }
            return;
        }
    }

    if (thisBucket->numNodes == thisBucket->bucketSize) {
        // realloc
        thisBucket->bucketSize += 10;
        thisBucket->keyArray = (key_type*) realloc(thisBucket->keyArray, thisBucket->bucketSize * sizeof(key_type));
        thisBucket->valueArray = (value_type*) realloc(thisBucket->valueArray, thisBucket->bucketSize * sizeof(value_type));
    }
    thisBucket->keyArray[thisBucket->numNodes] = k;
    thisBucket->valueArray[thisBucket->numNodes] = v;
    thisBucket->numNodes++;

    printf("put 3\n");

}

value_type basic_get(key_type k) {
    printf("get 1\n");

    struct bucket *thisBucket = ht->bucketArray[hash_function(k, tableSize)];

    printf("get 2\n");

    for (int i = 0; i < thisBucket->bucketSize; i++) {
        printf("get index %d\n", i);
        if (thisBucket->keyArray[i] == k) {
            return thisBucket->valueArray[i];
        }
    }

    printf("get 3\n");

    return 0;
}

void free_ht() {
    for (int i = 0; i < tableSize; i++) {
        free(ht->bucketArray[i]->keyArray);
        free(ht->bucketArray[i]->valueArray);
        free(ht->bucketArray[i]);
    }
    free(ht->bucketArray);
    free(ht);
}

void main() {
    initHashtable();
    basic_put(1, 2);
    basic_put(3, 4);
    printf("key = 1, value = %d", basic_get(1));
    printf("key = 3, value = %d", basic_get(3));
    printf("key = 2, value = %d", basic_get(2));
    free_ht();
}