# the name of the target operating system
set(CMAKE_SYSTEM_NAME MetisX)

set(METISX_MU_COMPILER_PATH /mnt/media/mu_compiler/llvm-p5-malloc)
set(LLVM_PATH ${METISX_MU_COMPILER_PATH})

# which compilers to use for C and C++
set(CMAKE_CXX_COMPILER ${LLVM_PATH}/bin/clang++)
set(CMAKE_C_COMPILER ${LLVM_PATH}/bin/clang)
set(CMAKE_ASM_COMPILER ${LLVM_PATH}/bin/clang)
set(CMAKE_AR ${LLVM_PATH}/bin/llvm-ar)

set(FP_FLAG -nofp)

# set compiler options
macro(metisx_compiler_clang lang)
  set(CMAKE_${lang}_FLAGS_INIT "-march=metis+m \
                                -ffreestanding \
                                -nostdlib \
                                -nodefaultlibs \
                                -fno-exceptions \
                                -ffunction-sections \
                                -fdata-sections \
                                -fno-rtti \
                                -Werror=return-type \
                                -D_MU_ -D_SLAVE_ \
                                -I${METISX_MU_COMPILER_PATH} \
                                -I${METISX_MU_COMPILER_PATH}/include \
                                -I${METISX_MU_COMPILER_PATH}/picolibc${FP_FLAG} \
                                -I${METISX_MU_COMPILER_PATH}/picolibc${FP_FLAG}/include \
                                -I${METISX_MU_COMPILER_PATH}/libcxx${FP_FLAG}/include/c++/v1 \
                                ")
endmacro()
metisx_compiler_clang(ASM)
metisx_compiler_clang(C)
metisx_compiler_clang(CXX)

#set linker options
set(MU_LINKER_FLAGS "-lclang_rt.builtins-metis\
                    -z max-page-size=4\
                    -lc\
                    -lc++ \
                    -lc++abi \
                    -L${METISX_MU_COMPILER_PATH}/../llvm-p5-cxxabi/libcxxabi${FP_FLAG}/lib \
                    -L${METISX_MU_COMPILER_PATH}/libcxx${FP_FLAG}/lib \
                    -Wl,--gc-sections\
                    -L${METISX_MU_COMPILER_PATH}/builtins${FP_FLAG}/lib/linux \
                    --ld-path=${METISX_MU_COMPILER_PATH}/bin/ld.lld\
                    -L${METISX_MU_COMPILER_PATH}/builtins${FP_FLAG}\
                    -L${METISX_MU_COMPILER_PATH}/picolibc${FP_FLAG}/newlib\
                    ")
set(CMAKE_EXE_LINKER_FLAGS ${MU_LINKER_FLAGS})
