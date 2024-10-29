#include "metisx/metisx.h"
#include "sim/sim.h"
#include "library/wrapper/sim_wrapper.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <complex>
#include <nlohmann/json.hpp>

static constexpr uint64_t DeviceId = 0;
static constexpr uint64_t NumSub = 1;
static constexpr uint64_t ArrSize = 1 << 5;

static constexpr uint64_t testCount = 1;
static constexpr uint64_t numCluster = 1;
static constexpr uint64_t batchSize = 1;

inline std::string getSoPath(std::string_view filename)
{
    if (filename.size() < 3)
    {
        printf("filename is too short\n");
        assert(0);
    }

    if (filename.substr(filename.size() - 3, 3) == ".mo")
    {
        std::string soFileName = std::string(filename.substr(0, filename.size() - 3));
        soFileName += "_gcc.so";
        return soFileName;
    }

    printf("filename is not .mo\n");
    return "";
}

void init(std::string_view filename, std::function<void(::metisx::MetisxConfig &config)> setup)
{
    metisx::sim::arch::SimArch::getInstance().setPhase(metisx::sys::Phase::P5);

    auto sofile = getSoPath(filename);
    ::metisx::MetisxConfig config(metisx::Mode::C_MODEL);

    ::metisx::library::wrapper::registerCModelFromSo(sofile.c_str());
    if (std::getenv("METISX_RTL_DUMP") != nullptr)
    {
        config.rtlDumpCfg = metisx::RtlDumpCfg::create();
    }

    if (setup != nullptr)
    {
        setup(config);
    }

    metisx::initMetisX(&config);
}

using json = nlohmann::json;

void read_query_json(const std::string& filename, std::vector<int>& indices, std::vector<double>& values) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << filename << ", Could not open the file!" << std::endl;
        return;
    }

    json j;
    file >> j;

    indices = j["indices"].get<std::vector<int>>();
    values = j["values"].get<std::vector<double>>();

    file.close();
}

void read_doc_json(const std::string& filename, std::vector<std::vector<int>>& indices, std::vector<std::vector<double>>& values) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << filename << ", Could not open the file!" << std::endl;
        return;
    }

    json j;
    file >> j;

    int doc_count = j["doc_count"];
    indices = j["indices"];
    values = j["values"];

    file.close();
}

int main()
{
    std::string filename = "/home/smshin/workspace/pg_rs/pgvecto.rs/mu_offloading/build/debug/kernel/inverted_index_kernel.mo";

    init(filename.c_str(), []([[maybe_unused]] metisx::MetisxConfig &config)
         {
                                      metisx::sim::arch::SimArch::getInstance().setPhase(metisx::sim::arch::HwPhase::P5);
                                      config.mode = metisx::Mode::C_MODEL; });

    std::vector<std::vector<int>> docIndicesRead;
    std::vector<std::vector<double>> docValuesRead;
    std::vector<int> queryIndicesRead;
    std::vector<double> queryValuesRead;

    read_doc_json("/home/smshin/workspace/pg_rs/pgvecto.rs/mu_offloading/data/docs_sparse_vector.json", docIndicesRead, docValuesRead);
    read_query_json("/home/smshin/workspace/pg_rs/pgvecto.rs/mu_offloading/data/query_sparse_vector.json", queryIndicesRead, queryValuesRead);

    auto docSize = std::make_unique<int[]>(testCount);
    auto querySize = std::make_unique<int[]>(testCount);

    auto docIndices = std::make_unique<std::complex<int>[][ArrSize]>(testCount);
    auto docValues = std::make_unique<std::complex<double>[][ArrSize]>(testCount);
    auto queryIndices = std::make_unique<std::complex<int>[][ArrSize]>(testCount);
    auto queryValues = std::make_unique<std::complex<double>[][ArrSize]>(testCount);

    for (uint64_t i = 0; i < testCount; i++)
    {
        for (uint64_t j = 0; j < docIndicesRead.size(); j++)
        {
            docIndices[i][j] = std::complex<int>(docIndicesRead[i][j], 0);
            docValues[i][j] = std::complex<double>(docValuesRead[i][j], 0.0);
        }
        for (uint64_t j = 0; j < queryIndicesRead.size(); j++)
        {
            queryIndices[i][j] = std::complex<int>(queryIndicesRead[j], 0);
            queryValues[i][j] = std::complex<double>(queryValuesRead[j], 0.0);
        }

        docSize[i] = docIndicesRead.size();
        querySize[i] = queryIndicesRead.size();
    }

    auto context = metisx::runtime::createContext(DeviceId);
    auto job = context->createJob(NumSub);
    job->load(filename.c_str());
    auto map = job->buildMap(testCount);

    assert(numCluster <= 4);
    auto clusterBitmap = metisx::makeBitmap(0, numCluster);

    map->setClusterBitmap(clusterBitmap);
    map->setBatchSize(batchSize);

    map->setInput(docIndices, docValues, queryIndices, queryValues, docSize, querySize);
    map->execute(docIndices, docValues, queryIndices, queryValues, docSize, querySize);
    map->synchronize();

    return 0;
}