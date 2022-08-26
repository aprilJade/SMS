#include "simpleHashTable.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static unsigned int hash65599(const char* key)
{
    unsigned int result = 0;
    int len = strlen(key);

    // unsigned int poly = 0xEDB88320;
    for (int i = 0; i < len; i++)
    {
        // poly = (poly << 1) | (poly >> 31);
        result = 65599 * result + key[i];
    }
    return result;
}

SHashTable* NewHashTable()
{
    SHashTable* result = (SHashTable*)malloc(sizeof(SHashTable));
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
        result->table[i] = (SHashNode*)malloc(sizeof(SHashNode));
    result->count = 0;
    return result;
}

int AddKeyValue(const char* key, const void* value, const size_t valueSize, SHashTable* hashTable)
{
    assert(key && value && hashTable);
    
    unsigned int hashedKey = hash65599(key);
    SHashNode* list = hashTable->table[hashedKey % HASH_TABLE_SIZE];
    while (list->next != NULL)
    {
        if (list->key == hashedKey) // there is key already
            return 1;
        list = list->next;
    }
    SHashNode* node = (SHashNode*)malloc(sizeof(SHashNode));
    node->key = hashedKey;
    node->value = malloc(valueSize);
    memcpy(node->value, value, valueSize);
    node->next = NULL;
    list->next = node;

    return 0;
}

void* GetValueByKey(const char* key, SHashTable* hashTable)
{
    assert(key && hashTable);
    unsigned int hashedKey = hash65599(key);
    SHashNode* list = hashTable->table[hashedKey % HASH_TABLE_SIZE]->next;

    while (list != NULL)
    {
        if (list->key == hashedKey)
            return list->value;
        list = list->next;
    }
    return NULL;
}

void ReleaseHashTable(SHashTable* hashTable)
{
    assert(hashTable);
    SHashNode* prev;
    SHashNode* list;
    for (int i = 0; i < HASH_TABLE_SIZE; i++)
    {
        list = hashTable->table[i]->next;
        while (list)
        {
            prev = list;
            list = list->next;
            free(prev->value);
            free(prev);
        }
        free(hashTable->table[i]);
        hashTable->table[i] = NULL;
    }
}

int UpdateValue(const char* key, const void* newValue, SHashTable* hashTable)
{
    // Not implemented yet
}