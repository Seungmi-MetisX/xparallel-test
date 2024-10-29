#include <complex>

#include "mu.h"

void inverted_index_kernel(size_t *usize_args, uint32_t* u32_args, double* f64_args, void *result)
{
    printf("[kernel] inverted index kernel started\n");
    auto taskId = getTaskIdx();
    printf("[kernel] taskId = %d\n", taskId);

    auto usizeargs = usize_args;
    auto index_len = *(size_t *)usizeargs;
    usizeargs = (size_t *)usizeargs + 1;
    printf("[kernel] index_len = %u\n", index_len);

    auto offsets_len = *(size_t *)usizeargs;
    usizeargs = (size_t *)usizeargs + 1;
    printf("[kernel] offsets_len = %d\n", offsets_len);

    auto scores_len = *(size_t *)usizeargs;
    usizeargs = (size_t *)usizeargs + 1;
    printf("[kernel] scores_len = %d\n", scores_len);

    auto query_len = *(size_t *)usizeargs;
    usizeargs = (size_t *)usizeargs + 1;
    printf("[kernel] query_len = %d\n", query_len);

    auto u32args = u32_args;
    auto indexes = (uint32_t *)u32args;
    u32args = (uint32_t *)((uint32_t *)u32args + index_len);

    auto offsets = (uint32_t *)u32args;
    u32args = (uint32_t *)((uint32_t *)u32args + offsets_len);

    auto query_index = (uint32_t *)u32args;
    u32args = (uint32_t *)((uint32_t *)u32args + query_len);

    auto f64args = f64_args;
    auto scores = (double *)f64args;
    f64args = (double *)((double *)f64args + scores_len);

    auto query_value = (double *)f64args;

    auto result_ptr = static_cast<double *>(result);

    for (int i = 0; i < query_len; i++)
    {
        auto token = query_index[i];
        auto val = query_value[i];
        auto start = offsets[token];
        auto end = offsets[token + 1];
        printf("[kernel] token = %d, val = %f, start = %d, end = %d\n", token, val, start, end);
        for (int j = start; j < end; j++)
        {
            result_ptr[indexes[j]] += scores[j] * val;
            printf("[kernel] result[%d] = %f\n", indexes[j], result_ptr[indexes[j]]);
        }
    }
    std::sort(result_ptr, result_ptr + index_len);

    double dotResult = 0.0f;
    printf("[kernel] inverted index kernel finished. result = %f\n", dotResult);
}

MU_KERNEL_INIT(inverted_index_kernel)