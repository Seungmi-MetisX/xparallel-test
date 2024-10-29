#pragma once
#include "metisx/metisx.h"
#include "rust/cxx.h"
#include <string>

// #define SHARED_PTR_TEST
// #define CPP_TEST

namespace xparallel
{
    using c_void = void;

    class Context;

#ifdef SHARED_PTR_TEST
    std::shared_ptr<Context> g_context = nullptr;
#else
    Context *g_context = nullptr;
#endif
    std::mutex g_context_mutex;

    class Context
    {
    public:
        Context();
        ~Context();

        metisx::runtime::Job *get_job() const { return job_; }
        metisx::runtime::Context *get_context() const { return context_; }

        void load_module(metisx::Module *module) const
        {
            job_->load(module);
        }
        metisx::runtime::Parallel *build_parallel(const metisx::Function *func, uint count)
        {
#if 0
            parallel_ = job_->buildParallel(count);
            return parallel_;
#else
            return job_->buildParallel(count);
#endif
        }

    private:
        metisx::runtime::Context *context_;
        metisx::runtime::Job *job_;
        metisx::runtime::Parallel *parallel_;
        metisx::Mode mode_;
        size_t taskCount_;
        int deviceId_;
        int numSub_;
        metisx::sys::Phase phase_;
    };

    class Module
    {
    public:
        Module(const Context &context, const std::string &path);

        metisx::Function *get_function(const std::string &name) const { return module_->function(name.c_str()); }

    private:
        metisx::Module *module_;
    };

    class Function
    {
    public:
        Function(const Module &module, const std::string &name);
        metisx::Function *get_function() { return function_; }

    private:
        metisx::Function *function_;
    };

    class Parallel
    {
    public:
        Parallel(metisx::runtime::Parallel *p) : parallel_(p) {}

        metisx::runtime::Parallel *get_parallel() { return parallel_; }

        // template <typename... Args>
        //  void execute(Args... args) { parallel_->execute(args...); }
        void execute(void *arg, void *arg2, void *arg3, void *arg4);
        void synchronize();

    private:
        metisx::runtime::Parallel *parallel_ = nullptr;
    };
#ifdef SHARED_PTR_TEST
    std::shared_ptr<Context> create_context();
#else
    std::unique_ptr<Context> create_context();
#endif
    void destroy_context(Context *context);

#ifdef SHARED_PTR_TEST
    void set_context(std::shared_ptr<Context> context);
#else
    void set_context(Context &context);
#endif

#ifdef CPP_TEST
    std::unique_ptr<Module> create_module(std::string &path);
#else
    std::unique_ptr<Module> create_module(rust::Str path);
#endif

    void destroy_module(Module *module);

#ifdef CPP_TEST
    std::unique_ptr<Function> create_function(const Module &module, std::string &name);
#else
    std::unique_ptr<Function> create_function(const Module &module, rust::Str name);
#endif
    void destroy_function(Function *function);

    void *alloc(size_t size);
    void free(void *ptr);
    void copy_host_to_device(void *ptr, size_t size);
    void copy_device_to_host(void *ptr, size_t size);

#ifdef SHARED_PTR_TEST
    std::shared_ptr<Parallel> build_parallel(Function &func);
#else
    std::unique_ptr<Parallel> build_parallel(Function &func);
#endif
    std::unique_ptr<Parallel> build_parallel_test(Function &func, void *arg, void *arg2, void *arg3, void *arg4);

#ifdef SHARED_PTR_TEST
    void parallel1_execute(std::shared_ptr<Parallel> parallel, void *arg);
    void parallel2_execute(std::shared_ptr<Parallel> parallel, void *arg, void *arg2);
    void parallel3_execute(std::shared_ptr<Parallel> parallel, void *arg, void *arg2, void *arg3);
    void parallel4_execute(std::shared_ptr<Parallel> parallel, void *arg, void *arg2, void *arg3, void *arg4);
#else
    void parallel1_execute(Parallel &parallel, void *arg);
    void parallel2_execute(Parallel &parallel, void *arg, void *arg2);
    void parallel3_execute(Parallel &parallel, void *arg, void *arg2, void *arg3);
    void parallel4_execute(Parallel &parallel, void *arg, void *arg2, void *arg3, void *arg4);
#endif
} // xparallel