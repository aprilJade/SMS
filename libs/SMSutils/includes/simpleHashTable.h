#ifndef SIMPLE_HASH_TABLE_H
#define SIMPLE_HASH_TABLE_H
#define HASH_TABLE_SIZE 119
#include <stddef.h>

typedef struct SHashNode
{   
    int key;
    void* value;
    struct SHashNode* next;
} SHashNode;

typedef struct SHashTable
{
    SHashNode* table[HASH_TABLE_SIZE];
    int count;
} SHashTable;

SHashTable* NewHashTable();

int AddKeyValue(const char* key, const void* value, const size_t valueSize, SHashTable* hashTable);
void* GetValueByKey(const char* key, SHashTable* hashTable);
void ReleaseHashTable(SHashTable* hashTable);
int UpdateValue(const char* key, const void* newValue, SHashTable* hashTable);

#endif