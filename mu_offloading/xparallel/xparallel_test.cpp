#include "xparallel.hpp"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int main()
{
    auto indexes_file_path = "/home/smshin/workspace/pg-xvector/build/debug/dep/postgres/bin/vectorDB/pg_vectors/indexes/0000000000000000000000000000000066fa6e6b7b0b80960000815900008336/sealed_segments/1/indexes";
    auto offsets_file_path = "/home/smshin/workspace/pg-xvector/build/debug/dep/postgres/bin/vectorDB/pg_vectors/indexes/0000000000000000000000000000000066fa6e6b7b0b80960000815900008336/sealed_segments/1/offsets";
    auto scores_file_path = "/home/smshin/workspace/pg-xvector/build/debug/dep/postgres/bin/vectorDB/pg_vectors/indexes/0000000000000000000000000000000066fa6e6b7b0b80960000815900008336/sealed_segments/1/scores";

    std::ifstream indexes_file(indexes_file_path);
    std::ifstream offsets_file(offsets_file_path);
    std::ifstream scores_file(scores_file_path);

    if (!indexes_file.is_open() || !offsets_file.is_open() || !scores_file.is_open())
    {
        std::cerr << "Error opening one of the files." << std::endl;
        return -1;
    }

    json indexes_json;
    json offsets_json;
    json scores_json;

    indexes_file >> indexes_json;
    offsets_file >> offsets_json;
    scores_file >> scores_json;

    auto indexes = indexes_json.get<std::vector<uint32_t>>();
    auto offsets = offsets_json.get<std::vector<uint32_t>>();
    auto scores = scores_json.get<std::vector<double>>();

    // std::cout << "Indexes JSON: " << indexes_json.dump(4) << std::endl;
    // std::cout << "Offsets JSON: " << offsets_json.dump(4) << std::endl;
    // std::cout << "Scores JSON: " << scores_json.dump(4) << std::endl;

    std::vector<uint32_t> query_index = {27, 47, 398, 581, 3444, 7986, 33938};
    std::vector<double> query_value = {
        0.1692243069410324,
        0.06639421731233597,
        0.0892706885933876,
        0.09344992786645889,
        0.16868501901626587,
        0.2873534560203552,
        0.2923230528831482};

    auto context = xparallel::create_context();
    xparallel::set_context(*context);

    auto module_path = std::string("/home/smshin/workspace/pg_rs/pgvecto.rs/mu_offloading/build/debug/kernel/inverted_index_kernel.mo");
    auto module = xparallel::create_module(module_path);

    auto function_name = std::string("inverted_index_kernel");
    auto function = xparallel::create_function(*module, function_name);

    size_t index_len = indexes_json.size();
    size_t offset_len = offsets_json.size();
    size_t score_len = scores_json.size();
    size_t query_len = query_index.size();

    size_t index_size = index_len * sizeof(uint32_t);
    size_t offset_size = offset_len * sizeof(uint32_t);
    size_t score_size = score_len * sizeof(double);

    size_t query_index_size = query_len * sizeof(uint32_t);
    size_t query_value_size = query_len * sizeof(double);
    size_t query_size = query_index_size + query_value_size;

    size_t len_info_size = 4 * sizeof(size_t);
    size_t uint_args_size = index_size + offset_size + query_index_size;
    size_t double_args_size = score_size + query_value_size;

    void *len_info_mem = xparallel::alloc(len_info_size);
    void *uint_args_mem = xparallel::alloc(uint_args_size);
    void *double_args_mem = xparallel::alloc(double_args_size);

    size_t result_mem_size = sizeof(double) * 154; // doc count =  154
    void *result_mem = xparallel::alloc(result_mem_size);

    size_t byte_offset = 0;

    size_t *len_info_ptr = reinterpret_cast<size_t *>(len_info_mem);
    *len_info_ptr = index_len;
    len_info_ptr += 1;
    *len_info_ptr = offset_len;
    len_info_ptr += 1;
    *len_info_ptr = score_len;
    len_info_ptr += 1;
    *len_info_ptr = query_len;

    uint32_t *uint_args_ptr = reinterpret_cast<uint32_t *>(uint_args_mem);
    std::memcpy(uint_args_ptr, indexes.data(), index_size);
    uint_args_ptr += index_len;
    std::memcpy(uint_args_ptr, offsets.data(), offset_size);
    uint_args_ptr += offset_len;
    std::memcpy(uint_args_ptr, query_index.data(), query_index_size);

    double *double_args_ptr = reinterpret_cast<double *>(double_args_mem);
    std::memcpy(double_args_ptr, scores.data(), score_size);
    double_args_ptr += score_len;
    std::memcpy(double_args_ptr, query_value.data(), query_value_size);

    xparallel::copy_host_to_device(len_info_mem, len_info_size);
    xparallel::copy_host_to_device(uint_args_mem, uint_args_size);
    xparallel::copy_host_to_device(double_args_mem, double_args_size);
    xparallel::copy_host_to_device(result_mem, result_mem_size);

    auto parallel = xparallel::build_parallel(*function);
    xparallel::parallel4_execute(*parallel, len_info_mem, uint_args_mem, double_args_mem, result_mem);
    // parallel->synchronize();
}