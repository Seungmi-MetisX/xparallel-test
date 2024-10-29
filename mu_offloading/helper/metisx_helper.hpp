#pragma once
#include "metisx/metisx.h"

#include <vector>

namespace mxhelper
{
    typedef enum mxHelperStatus
    {
        MX_HELPER_SUCCESS = 0,
        MX_HELPER_ERROR = 1
    } mxHelperStatus;

    constexpr metisx::Mode defaultMode = metisx::Mode::C_MODEL;
    constexpr size_t defaultTaskCount = 0;
    constexpr int defaultDeviceId = 0;
    constexpr int defaultNumSub = -1;
    constexpr metisx::sys::Phase defaultPhase = metisx::sys::Phase::P5;
    constexpr uint64_t arrSize = 1 << 5;

    class MetisxHelper
    {
    public:
        using UintVec = std::vector<uint>;
        using UintVecArray = std::vector<std::vector<uint>>;
        using DoubleVec = std::vector<double>;
        using DoubleVecArray = std::vector<std::vector<double>>;

        MetisxHelper(uint testCount, uint deviceId, uint numSub);
        MetisxHelper();
        ~MetisxHelper();

        void tearDown();
        void init(metisx::Mode mode, bool gaiaEnable);
        void check();

        void buildMap(uint count);
        void buildParallel(uint count);

        // void executeMap(const UintVecArray &docIndciesList, const DoubleVecArray &docValuesList, const UintVec &queryIndices, const DoubleVec &queryValues);
        void executeMap(const std::vector<std::vector<uint32_t>> &docIndicesList, const std::vector<std::vector<double>> &docValuesList, const std::vector<uint32_t> &queryIndices, const std::vector<double> &queryValues);
        void executeParallel();

        void setDocuments(const uint *const docIncesList, const double *const docValuesList, const uint docCount, const uint *const docDim);
        void setQuery(const uint *const queryIndices, const double *const queryValues, const uint queryDim);

    private:
        void initMetisX(metisx::Mode mode);
        void createContext();
        void createJob();
        void loadModule(std::string &modulePath);

    private:
        metisx::runtime::Context *context_;
        metisx::runtime::Job *job_;
        metisx::runtime::Map *map_;
        metisx::runtime::Parallel *parallel_;
        size_t taskCount_ = defaultTaskCount;
        int deviceId_ = defaultDeviceId;
        int numSub_ = defaultNumSub;
        metisx::sys::Phase phase_;

        // std::vector<std::vector<uint32_t>> docIndicesList_;
        // std::vector<std::vector<double>> docValuesList_;
        // std::vector<uint32_t> queryIndices_;
        // std::vector<double> queryValues_;

        uint32_t docCount_;
        uint64_t docSize_;
        uint32_t *docDims_;
        uint32_t *docIndicesList_;
        double *docValuesList_;
        uint32_t *queryIndices_;
        double *queryValues_;
        uint32_t queryDim_;
        double *results_;
    
    };
} // namespace mxhelper