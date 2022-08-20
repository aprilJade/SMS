#ifndef SIMPLE_HASH_TABLE_H
#define SIMPLE_HASH_TABLE_H
#define HASH_TABLE_SIZE 128

typedef struct SHashNode
{   
    int key;
    char* value;
    struct SHashNode* next;
} SHashNode;

typedef struct SHashTable
{
    SHashNode* table[HASH_TABLE_SIZE];
    int count;
} SHashTable;

SHashTable* NewHashTable();

// Add new key with new value
int AddKeyValue(const char* key, const char* value, SHashTable* hashTable);
// Lookup table and get value. if there is no value, return NULL
char* GetValueByKey(const char* key, SHashTable* hashTable);
// Free safely
void ReleaseHashTable(SHashTable* hashTable);
// Update 
int UpdateValue(const char* key, const char* newValue, SHashTable* hashTable);

#endif