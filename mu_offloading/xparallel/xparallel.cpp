#pragma once
#include "xparallel.hpp"

#include "metisx/metisx.h"
#include "library/library_instance.hpp"
#include "library/wrapper/sim_wrapper.hpp"
#include "sim/arch/sim_arch.hpp"

#include <vector>

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

namespace xparallel
{
    constexpr metisx::Mode defaultMode = metisx::Mode::SIM;
    constexpr size_t defaultTaskCount = 0;
    constexpr int defaultDeviceId = 0;
    constexpr int defaultNumSub = -1;

    inline std::string getSoPath(const std::string &filename)
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

        std::cout << "filename " << filename << " is not .mo" << std::endl;
        return "";
    }

    Module::Module(const Context &context, const std::string &path)
        : module_(nullptr)
    {
        auto sofile = getSoPath(path);
        DEBUG_PRINTLN("sofile: %s\n", sofile.data());
        metisx::library::wrapper::registerCModelFromSo(sofile.data());
        DEBUG_PRINTLN("registerCModelFromSo done");

        module_ = metisx::createModule(path.c_str());
        DEBUG_PRINTLN("createModule done");

        // context.get_job()->load(module_);
        context.load_module(module_);
        DEBUG_PRINTLN("load module done");
    }

    Function::Function(const Module &module, const std::string &name)
    {
        function_ = module.get_function(name);
        if (function_)
        {
            DEBUG_PRINTLN("get function %s done", name.c_str());
        }
        else
        {
            DEBUG_PRINTLN("get function %s failed", name.c_str());
        }
    }

    // void Parallel::synchronize()
    // {
    //     parallel_->synchronize();
    // }

    Context::Context()
        : context_(nullptr),
          job_(nullptr),
          parallel_(nullptr),
          mode_(defaultMode),
          taskCount_(defaultTaskCount),
          deviceId_(defaultDeviceId),
          numSub_(defaultNumSub)
    {
        metisx::sim::arch::SimArch::getInstance().setPhase(metisx::sys::Phase::P5);
        metisx::sim::arch::SimArch::getInstance().setGaiaSupport(false);
        metisx::MetisxConfig config(mode_);
        metisx::initMetisX(&config);

        context_ = metisx::runtime::createContext(deviceId_);
        if (numSub_ < 0)
        {
            numSub_ = context_->remainSub();
        }
        job_ = context_->createJob();

        // auto threadBitmap = metisx::makeBitmap(0, numThread);
        // context->setThreadBitmap(threadBitmap);

        DEBUG_PRINTLN("create xparallel::Context done");
    }
    Context::~Context()
    {
        DEBUG_PRINTLN("delete context\n");
    }

    void Parallel::execute(void *arg, void *arg2, void *arg3, void *arg4)
    {
        DEBUG_PRINTLN("execute");
        DEBUG_PRINTLN("parallel_ = %p", parallel_);
        parallel_->execute(arg, arg2, arg3, arg4);
        DEBUG_PRINTLN("execute done");
    }
    void Parallel::synchronize()
    {
        DEBUG_PRINTLN("sync");
        parallel_->synchronize();
        DEBUG_PRINTLN("sync done");
    }
#ifdef SHARED_PTR_TEST
    std::shared_ptr<Context> create_context()
    {
        return std::make_shared<Context>();
    }
#else
    std::unique_ptr<Context> create_context()
    {
        return std::make_unique<Context>();
    }
#endif
    void destroy_context(Context *context)
    {
        delete context;
    }

#ifdef SHARED_PTR_TEST
    void set_context(std::shared_ptr<Context> context)
    {
        std::lock_guard<std::mutex> lock(g_context_mutex);
        g_context = context;
    }
#else
    void set_context(Context &context)
    {
        std::lock_guard<std::mutex> lock(g_context_mutex);
        g_context = &context;
    }
#endif

    const Context &get_context()
    {
        return *g_context;
    }

#ifdef CPP_TEST
    std::unique_ptr<Module> create_module(std::string &path)
#else
    std::unique_ptr<Module> create_module(rust::Str path)
#endif
    {
        if (g_context == nullptr)
        {
            DEBUG_PRINTLN("g_context is nullptr");
            assert(0);
        }
#ifdef CPP_TEST
        auto path_string = path;
#else
        auto path_string = static_cast<std::string>(path);
#endif
        return std::make_unique<Module>(get_context(), path_string);
    }
    void destroy_module(Module *module) {}

#ifdef CPP_TEST
    std::unique_ptr<Function> create_function(const Module &module, std::string &name)
#else
    std::unique_ptr<Function> create_function(const Module &module, rust::Str name)
#endif
    {
#ifdef CPP_TEST
        auto name_string = name;
#else
        auto name_string = static_cast<std::string>(name);
#endif
        return std::make_unique<Function>(module, name_string);
    }
    void destroy_function(Function *function) {}

    void *alloc(size_t size)
    {
        if (g_context == nullptr)
        {
            DEBUG_PRINTLN("g_context is nullptr");
            assert(0);
        }
        DEBUG_PRINTLN("alloc size: %d", size);
        auto mem = (void *)g_context->get_context()->ioMemAlloc(size);
    }
    void free(void *ptr) {}
    void copy_host_to_device(void *ptr, size_t size)
    {
        metisx::runtime::syncToDevice(ptr, size);
    }
    void copy_device_to_host(void *ptr, size_t size)
    {
        metisx::runtime::syncFromDevice((void *)ptr, size);
    }

    std::unique_ptr<Parallel> build_parallel_test(Function &func, void *arg, void *arg2, void *arg3, void *arg4)
    {
        if (g_context == nullptr)
        {
            DEBUG_PRINTLN("g_context is nullptr");
            assert(0);
        }

#if 0
        auto parallel = g_context->build_parallel(func.get_function(), 1);
        parallel->execute(arg, arg2, arg3, arg4);
        parallel->synchronize();
        return std::make_unique<Parallel>(parallel);
#else
        auto parallel = std::make_unique<Parallel>(g_context->build_parallel(func.get_function(), 1));
        parallel->execute(arg, arg2, arg3, arg4);
        parallel->synchronize();
        return parallel;
#endif
    }

#ifdef SHARED_PTR_TEST
    std::shared_ptr<Parallel> build_parallel(Function &func)
    {
        if (g_context == nullptr)
        {
            DEBUG_PRINTLN("g_context is nullptr");
            assert(0);
        }
        return std::make_shared<Parallel>(g_context->build_parallel(func.get_function(), 1));
    }
#else
    std::unique_ptr<Parallel> build_parallel(Function &func)
    {
        if (g_context == nullptr)
        {
            DEBUG_PRINTLN("g_context is nullptr");
            assert(0);
        }
        return std::make_unique<Parallel>(g_context->build_parallel(func.get_function(), 1));
    }
#endif

#ifdef SHARED_PTR_TEST
    void parallel1_execute(std::shared_ptr<Parallel> parallel, void *arg)
    {
        // parallel.execute(arg);
    }

    void parallel2_execute(std::shared_ptr<Parallel> parallel, void *arg, void *arg2)
    {
        // parallel.execute(arg, arg2);
    }
    void parallel3_execute(std::shared_ptr<Parallel> parallel, void *arg, void *arg2, void *arg3)
    {
        // parallel.execute(arg, arg2, arg3);
    }
    void parallel4_execute(std::shared_ptr<Parallel> parallel, void *arg, void *arg2, void *arg3, void *arg4)
    {
        void *arg_ptr = arg;
        auto index_len = *(size_t *)arg_ptr;
        arg_ptr = (size_t *)arg_ptr + 1;
        DEBUG_PRINTLN("index_len = %u\n", index_len);

        auto offsets_len = *(size_t *)arg_ptr;
        arg_ptr = (size_t *)arg_ptr + 1;
        DEBUG_PRINTLN("offsets_len = %d\n", offsets_len);

        auto scores_len = *(size_t *)arg_ptr;
        arg_ptr = (size_t *)arg_ptr + 1;
        DEBUG_PRINTLN("scores_len = %d\n", scores_len);

        auto query_len = *(size_t *)arg_ptr;
        arg_ptr = (size_t *)arg_ptr + 1;
        DEBUG_PRINTLN("query_len = %d\n", query_len);

        parallel->execute(arg, arg2, arg3, arg4);
        parallel->synchronize();
    }

#else
    void parallel1_execute(Parallel &parallel, void *arg)
    {
        // parallel.execute(arg);
    }

    void parallel2_execute(Parallel &parallel, void *arg, void *arg2)
    {
        // parallel.execute(arg, arg2);
    }
    void parallel3_execute(Parallel &parallel, void *arg, void *arg2, void *arg3)
    {
        // parallel.execute(arg, arg2, arg3);
    }
    void parallel4_execute(Parallel &parallel, void *arg, void *arg2, void *arg3, void *arg4)
    {
        void *arg_ptr = arg;
        auto index_len = *(size_t *)arg_ptr;
        arg_ptr = (size_t *)arg_ptr + 1;
        DEBUG_PRINTLN("index_len = %p %u\n", arg, index_len);

        auto offsets_len = *(size_t *)arg_ptr;
        arg_ptr = (size_t *)arg_ptr + 1;
        DEBUG_PRINTLN("offsets_len = %d\n", offsets_len);

        auto scores_len = *(size_t *)arg_ptr;
        arg_ptr = (size_t *)arg_ptr + 1;
        DEBUG_PRINTLN("scores_len = %d\n", scores_len);

        auto query_len = *(size_t *)arg_ptr;
        arg_ptr = (size_t *)arg_ptr + 1;
        DEBUG_PRINTLN("query_len = %d\n", query_len);

#if 1
        parallel.execute(arg, arg2, arg3, arg4);
        parallel.synchronize();
#else
        parallel.get_parallel()->execute(arg, arg2, arg3, arg4);
        // parallel.get_parallel()->synchronize();
#endif
    }
#endif
} // xparallel