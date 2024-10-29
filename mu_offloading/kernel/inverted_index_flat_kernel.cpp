#include <complex>

#include "mu.h"

void inverted_index_flat_kernel(uint32_t *docIndices, double *docValues, uint32_t *docDims, uint32_t *queryIndices, double *queryValues, uint queryDim, double *results)
{
    auto taskId = getTaskIdx();
    auto startIdx = 0;
    auto curDim = 0;
    int i = 0;
    for (; i < taskId; i++)
    {
        startIdx += docDims[i];
    }
    curDim = docDims[i];

    for (int i = startIdx; i < startIdx + curDim; i++)
    {
        printf("[task id: %d] Document (%d) %d %f ", taskId, i, (int)docIndices[i], docValues[i]);
    }

    for (int i = 0; i < queryDim; i++)
    {
        printf("[task id: %d] Query (%d) %d %f ", taskId, i, (int)queryIndices[i], queryValues[i]);
    }
    size_t docPos = startIdx;
    size_t queryPos = 0;

    float dotResult = 0.0f;

    while (docPos < startIdx + curDim && queryPos < queryDim)
    {
        int docIndex = docIndices[docPos];
        int queryIndex = queryIndices[queryPos];

        if (docIndex < queryIndex)
        {
            ++docPos;
        }
        else if (docIndex > queryIndex)
        {
            ++queryPos;
        }
        else
        { // docIndex == queryIndex
            printf("[task id: %d] index = %d, docValues[%d] = %f, queryValues[%d] = %f\n", taskId, docIndex, docPos, docValues[docPos], queryPos, queryValues[queryPos]);
            dotResult += docValues[docPos] * queryValues[queryPos];
            ++docPos;
            ++queryPos;
        }
    }
    printf("[task id: %d] inverted index kernel finished. result = %f\n", taskId, dotResult);
    results[taskId] = dotResult;
}

MU_KERNEL_INIT(inverted_index_flat_kernel)