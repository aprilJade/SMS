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
int AddKeyValue(const char* key, const char* value, SHashTable* hashTable);
char* GetValueByKey(const char* key, SHashTable* hashTable);
void ReleaseHashTable(SHashTable* hashTable);

int SetKeyValue(const char* key, const char* newValue, SHashTable* hashTable);

#endif