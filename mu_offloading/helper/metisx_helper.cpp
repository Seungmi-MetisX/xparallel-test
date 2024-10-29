#include "metisx_helper.hpp"
#include "metisx/metisx.h"
#include "library/library_instance.hpp"
#include "library/wrapper/sim_wrapper.hpp"
#include "sim/arch/sim_arch.hpp"

#include <iostream>
#include <complex>

#define DEBUG_PRINTLN(fmt, ...)                                                            \
    do                                                                                     \
    {                                                                                      \
        printf("[%s:%d (%s)] " fmt "\n", __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    } while (0);

#define DEBUG_PRINTF(...)    \
    do                       \
    {                        \
        printf(__VA_ARGS__); \
    } while (0);

namespace mxhelper
{
    inline std::string getSoPath(std::string_view filename)
    {
        if (filename.size() < 3)
        {
            std::cout << "filename is too short" << std::endl;
            assert(0);
        }

        if (filename.substr(filename.size() - 3, 3) == ".mo")
        {
            std::string soFileName = std::string(filename.substr(0, filename.size() - 3));
            soFileName += "_gcc.so";
            return soFileName;
        }

        std::cout << "filename is not .mo" << std::endl;
        return "";
    }

    MetisxHelper::MetisxHelper(uint taskCount, uint deviceId, uint numSub)
        : taskCount_(taskCount),
          deviceId_(deviceId),
          numSub_(numSub)
    {
        docCount_ = 0;
        docSize_ = 0;
        docIndicesList_ = nullptr;
        docValuesList_ = nullptr;
        queryIndices_ = nullptr;
        queryValues_ = nullptr;
        queryDim_ = 0;
        results_ = nullptr;
    }

    MetisxHelper::MetisxHelper()
    {
    }

    MetisxHelper::~MetisxHelper()
    {
        tearDown();
    }

    void MetisxHelper::tearDown()
    {
        context_->destroyJob(job_);
        metisx::runtime::destroyContext(context_);
        metisx::library::LibraryInstance::getInstance().destory();
    }

    void MetisxHelper::initMetisX(metisx::Mode mode)
    {
        DEBUG_PRINTLN("MetisxHelper::initMetisX [mode: %d]", static_cast<int>(mode));

        metisx::sim::arch::SimArch::getInstance().setPhase(metisx::sys::Phase::P5);
        DEBUG_PRINTLN("setPhase");

        metisx::sim::arch::SimArch::getInstance().setGaiaSupport(false);
        DEBUG_PRINTLN("setGaiaSupport");

        metisx::MetisxConfig config(mode);
        config.mode = metisx::Mode::SIM;
        metisx::initMetisX(&config);
        DEBUG_PRINTLN("initMetisX done");
    }

    void MetisxHelper::createContext()
    {
        DEBUG_PRINTLN("createContext start");
        context_ = metisx::runtime::createContext(deviceId_);
        // ASSERT_NE(context_, nullptr);

        DEBUG_PRINTLN("createContext done");
    }

    void MetisxHelper::createJob()
    {
        if (numSub_ < 0)
        {
            numSub_ = context_->remainSub();
        }
        job_ = context_->createJob();
        DEBUG_PRINTLN("createJob done");
        // ASSERT_NE(job_, nullptr);
        // ASSERT_EQ(job_->numSub(), numSub_);

        // auto threadBitmap = metisx::makeBitmap(0, numThread);
        // context->setThreadBitmap(threadBitmap);
    }

    void MetisxHelper::loadModule(std::string &modulePath)
    {
        auto sofile = getSoPath(modulePath.c_str());
        DEBUG_PRINTF("sofile: %s\n", sofile.c_str());
        metisx::library::wrapper::registerCModelFromSo(sofile.c_str());

        auto module = metisx::createModule(modulePath.c_str());
        job_->load(module);

        DEBUG_PRINTLN("loadModule done");
    }

    void MetisxHelper::init(metisx::Mode mode = defaultMode, bool gaiaEnable = false)
    {
        initMetisX(mode);

        createContext();
        createJob();

        std::string modulePath = "/home/smshin/workspace/pg_rs/pgvecto.rs/mu_offloading/build/debug/kernel/inverted_index_flat_kernel.mo";
        loadModule(modulePath);

        phase_ = static_cast<metisx::sys::Phase>(context_->deviceRevision());
        DEBUG_PRINTLN("metisx mode: %d, phase: %d\n", static_cast<int>(mode), static_cast<int>(phase_));

        DEBUG_PRINTLN("MetisxHelper init completed");
    }

    void MetisxHelper::check()
    {
        // ASSERT_NE(context_, nullptr);
        // ASSERT_GT(numSub_, 0);
    }

    void MetisxHelper::buildMap(uint count)
    {
        DEBUG_PRINTLN("MetisxHelper::buildMap");
        map_ = job_->buildMap(count);

        // assert(numCluster <= 4);
        // auto clusterBitmap = metisx::makeBitmap(0, numCluster);
        // map->setClusterBitmap(clusterBitmap);
        // uint32_t batchSize = 1;
        // map->setBatchSize(batchSize);
    }

    void MetisxHelper::buildParallel(uint count)
    {
        DEBUG_PRINTLN("MetisxHelper::buildParallel");
        parallel_ = job_->buildParallel(docCount_);
    }

    // void MetisxHelper::executeMap(const UintVecArray &docIndciesList, const DoubleVecArray &docValuesList, const UintVec &queryIndices, const DoubleVec &queryValues)
    void MetisxHelper::executeMap(const std::vector<std::vector<uint32_t>> &docIndicesList, const std::vector<std::vector<double>> &docValuesList, const std::vector<uint32_t> &queryIndices, const std::vector<double> &queryValues)
    {
        DEBUG_PRINTLN("MetisxHelper::executeMap");
    }

    void MetisxHelper::executeParallel()
    {
        results_ = (double *)context_->ioMemAlloc(sizeof(double) * docCount_);
        metisx::runtime::syncToDevice((void *)results_, sizeof(double) * docCount_);

        metisx::runtime::syncToDevice((void *)docIndicesList_, sizeof(uint32_t) * docSize_);
        metisx::runtime::syncToDevice((void *)docValuesList_, sizeof(double) * docSize_);
        metisx::runtime::syncToDevice((void *)docDims_, sizeof(uint32_t) * docCount_);

        metisx::runtime::syncToDevice((void *)queryIndices_, sizeof(uint32_t) * queryDim_);
        metisx::runtime::syncToDevice((void *)queryValues_, sizeof(double) * queryDim_);

        DEBUG_PRINTLN("MetisxHelper::executeParallel");

        parallel_->execute(docIndicesList_, docValuesList_, docDims_, queryIndices_, queryValues_, queryDim_, results_);
        parallel_->synchronize();

        metisx::runtime::syncFromDevice((void *)results_, sizeof(double) * docCount_);
        DEBUG_PRINTF("results: ");
        for (int i = 0; i < docCount_; i++)
        {
            DEBUG_PRINTF("%f ", results_[i]);
        }
        DEBUG_PRINTF("\n");
        DEBUG_PRINTLN("MetisxHelper::executeParallel done");
    }

    void MetisxHelper::setDocuments(const uint *const docIncesList, const double *const docValuesList, const uint docCount, const uint *const docDim)
    {
        docCount_ = docCount;
        docSize_ = 0;
        for (int i = 0; i < docCount_; i++)
        {
            docSize_ += docDim[i];
        }

        docIndicesList_ = (uint32_t *)context_->ioMemAlloc(sizeof(uint32_t) * docSize_);
        docValuesList_ = (double *)context_->ioMemAlloc(sizeof(double) * docSize_);
        docDims_ = (uint32_t *)context_->ioMemAlloc(sizeof(uint32_t) * docCount_);

        DEBUG_PRINTLN("docCount: %d\n", docCount_);
        auto idx = 0;
        for (int i = 0; i < docCount_; i++)
        {
            DEBUG_PRINTF("docDim[%d]: %d\n", i, docDim[i]);
            docDims_[i] = docDim[i];
            for (int j = 0; j < docDim[i]; j++)
            {
                docIndicesList_[idx] = docIncesList[idx];
                docValuesList_[idx] = docValuesList[idx];
                DEBUG_PRINTF("[%d, %f] ", docIndicesList_[idx], docValuesList_[idx]);
                idx++;
            }
            DEBUG_PRINTF("\n");
        }
    }

    void MetisxHelper::setQuery(const uint *const queryIndices, const double *const queryValues, const uint queryDim)
    {
        queryDim_ = queryDim;
        queryIndices_ = (uint32_t *)context_->ioMemAlloc(sizeof(uint32_t) * queryDim);
        queryValues_ = (double *)context_->ioMemAlloc(sizeof(double) * queryDim);

        DEBUG_PRINTF("queryDim: %d\n", queryDim);
        for (int i = 0; i < queryDim; i++)
        {
            queryIndices_[i] = queryIndices[i];
            queryValues_[i] = queryValues[i];
            DEBUG_PRINTF("[%d, %f] ", queryIndices_[i], queryValues_[i]);
        }
        DEBUG_PRINTF("\n");
    }

} // namespace mxhelper