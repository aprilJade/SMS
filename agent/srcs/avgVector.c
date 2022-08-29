#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef struct SAvgVec
{
    float* avgData;
    int idx;
    int curCount;
    int maxCount;
} SAvgVec;

SAvgVec* NewAvgVec(int size)
{
    assert(size > 0);
    SAvgVec* vec = (SAvgVec*)malloc(sizeof(SAvgVec));
    vec->avgData = (float*)malloc(sizeof(float) * size);
    vec->maxCount = size;
    vec->idx = 0;
    vec->curCount = 0;
    return vec;
}

float GetCurValue(SAvgVec* vec)
{
    assert(vec != NULL);
    return vec->avgData[vec->idx];
}

void InsertValue(float value, SAvgVec* vec)
{
    assert(vec != NULL);
    vec->avgData[vec->idx++] = value;
    vec->idx %= vec->maxCount;
}

float GetAvgValue(SAvgVec* vec)
{
    float result = 0.0;

    for (int i = 0; i < vec->curCount; i++)
        result = vec->avgData[i];
    return result / vec->curCount;
}