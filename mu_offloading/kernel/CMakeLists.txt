cmake_minimum_required(VERSION 3.26)
project(inverted_index_kernel)
set(CMAKE_CXX_STANDARD 17)
set(METISX_SDK_PATH /home/smshin/workspace/sdk)
message(STATUS ${METISX_SDK_PATH})
# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${METISX_SDK_PATH}/cmake)
# set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
# /home/smshin/workspace/pg_rs/pgvecto.rs/mu_offloading/kernel/cmake)
# set(CMAKE_TOOLCHAIN_FILE "${PATH_TO_XTEST}/cmake/toolchains/metisx.cmake")
# include(MetisXHelpers)

function(add_kernel name)
  set(options LIB EXE)
  set(oneValueArgs LINKER STARTUP ISA)
  set(multiValueArgs INCLUDE LIBRARY SOURCE DEFINITIONS)
  cmake_parse_arguments(METISX_MU "${options}" "${oneValueArgs}"
                        "${multiValueArgs}" ${ARGN})

  set(COMMAND python3 ${METISX_SDK_PATH}/mu/build_script/build.py --output
              ${name} --path ${CMAKE_CURRENT_LIST_DIR})
  # message(STATUS "!!!!!" ${COMMAND})

  if(${METISX_MU_LIB})
    set(COMMAND ${COMMAND} --lib)
  endif()

  if(DEFINED METISX_MU_ISA)
    set(COMMAND ${COMMAND} --isa ${METISX_MU_ISA})
  endif()

  if(DEFINED METISX_MU_LINKER)
    set(COMMAND ${COMMAND} --linker ${METISX_MU_LINKER})
  endif()

  if(DEFINED METISX_MU_STARTUP)
    set(COMMAND ${COMMAND} --startup ${METISX_MU_STARTUP})
  endif()

  if(DEFINED METISX_MU_LIBRARY)
    set(COMMAND ${COMMAND} --library)
    foreach(lib IN LISTS METISX_MU_LIBRARY)
      set(DEPENDENCIES ${DEPENDENCIES} ${lib})
      set(COMMAND ${COMMAND} ${lib})
    endforeach()
  endif()

  if(DEFINED METISX_MU_INCLUDE)
    set(COMMAND ${COMMAND} --include)
    foreach(inc IN LISTS METISX_MU_INCLUDE)
      set(COMMAND ${COMMAND} ${inc})
    endforeach()
  endif()

  if(DEFINED METISX_MU_SOURCE)
    set(COMMAND ${COMMAND} --source)
    foreach(src IN LISTS METISX_MU_SOURCE)
      set(COMMAND ${COMMAND} ${src})
    endforeach()
  endif()

  if(DEFINED METISX_MU_DEFINITIONS)
    set(COMMAND ${COMMAND} --definitions)
    foreach(def IN LISTS METISX_MU_DEFINITIONS)
      set(COMMAND ${COMMAND} ${def})
    endforeach()
  endif()

  message(STATUS "!!!!!" ${COMMAND})
  message(
    STATUS
      "!!!!!BUILD_${name} ${METISX_SDK_PATH}/mu/out/binary/${name}_binary.h to ${METISX_SDK_PATH}/metisx/firmware/bsp/mu_binary/${name}_binary.h"
  )
  add_custom_command(
    OUTPUT BUILD_${name}
    COMMAND ${COMMAND}
    COMMAND
      ${CMAKE_COMMAND} -E copy ${METISX_SDK_PATH}/mu/out/binary/${name}_binary.h
      ${METISX_SDK_PATH}/metisx/firmware/bsp/mu_binary/${name}_binary.h
    COMMAND
      ${CMAKE_COMMAND} -E copy
      ${METISX_SDK_PATH}/mu/out/binary/${name}_bin_info.hpp
      ${METISX_SDK_PATH}/metisx/firmware/bsp/mu_binary/${name}_bin_info.hpp
    COMMAND
      ${CMAKE_COMMAND} -E copy ${METISX_SDK_PATH}/mu/out/binary/${name}_text.bin
      ${METISX_SDK_PATH}/metisx/firmware/bsp/mu_binary/${name}_text.bin
    COMMAND
      ${CMAKE_COMMAND} -E copy
      ${METISX_SDK_PATH}/mu/out/binary/${name}_rodata.bin
      ${METISX_SDK_PATH}/metisx/firmware/bsp/mu_binary/${name}_rodata.bin
    # DEPENDS ${DEPENDENCIES} mu_lib_p4 mu_lib_rv mu_lib_rv_nofp DEPENDS
    # ${DEPENDENCIES}
  )

  add_custom_target(${name}_mu ALL DEPENDS BUILD_${name})

  add_library(${name} STATIC ${METISX_MU_SOURCE})
  if(${name} STRLESS "mu")
    add_dependencies(${name} ${name}_mu)
  endif()

  target_link_libraries(${name} PRIVATE ${METISX_MU_LIBRARY})
  # target_link_libraries(${name} PUBLIC fmt::fmt)
  target_compile_definitions(${name} PRIVATE _SIM_=1 ${METISX_MU_DEFINITIONS})
  target_include_directories(${name} PRIVATE ${METISX_MU_INCLUDE})

  # if (INSTALL_MODE STREQUAL "0")
  if(NOT ${METISX_MU_LIB})
    set(so_file ${name}_gcc)

    add_library(${so_file} SHARED ${METISX_MU_SOURCE})
    set_target_properties(${so_file} PROPERTIES PREFIX "")
    set_target_properties(${so_file} PROPERTIES LIBRARY_OUTPUT_DIRECTORY
                                                "${METISX_SDK_PATH}/mu/out")
    if(${so_file} STRLESS "mu")
      add_dependencies(${so_file} ${name}_mu)
    endif()

    target_link_directories(${so_file} PUBLIC
                            ${METISX_SDK_PATH}/out/build/Debug/metisx)
    target_link_directories(${so_file} PUBLIC
                            ${METISX_SDK_PATH}/out/build/Debug/mu/mu)
    target_link_libraries(${so_file} PRIVATE ${METISX_MU_LIBRARY})
    # target_link_libraries(${so_file} PUBLIC fmt::fmt metisx)
    target_link_libraries(${so_file} PUBLIC metisx)
    target_compile_definitions(${so_file} PRIVATE _SIM_=1
                                                  ${METISX_MU_DEFINITIONS})
    target_include_directories(${so_file} PRIVATE ${METISX_MU_INCLUDE})
  endif()
  # endif()

  target_link_directories(${name} PUBLIC
                          ${METISX_SDK_PATH}/out/build/Debug/metisx)
  target_link_directories(${name} PUBLIC
                          ${METISX_SDK_PATH}/out/build/Debug/mu/mu)

endfunction()

function(add_mu_kernel_exe name)
  add_kernel(${name} ISA RISCV EXE ${ARGN})
endfunction()

add_mu_kernel_exe(
  inverted_index_flat_kernel
  LINKER
  ${METISX_SDK_PATH}/mu/script/mu_linker_p5.ld
  STARTUP
  ${METISX_SDK_PATH}/mu/script/mu_startup_p5.s
  SOURCE
  inverted_index_flat_kernel.cpp
  INCLUDE
  ${METISX_SDK_PATH}/mu
  ${METISX_SDK_PATH}/metisx
  LIBRARY
  mu_lib_rv)

add_mu_kernel_exe(
  inverted_index_kernel
  LINKER
  ${METISX_SDK_PATH}/mu/script/mu_linker_p5.ld
  STARTUP
  ${METISX_SDK_PATH}/mu/script/mu_startup_p5.s
  SOURCE
  inverted_index_kernel.cpp
  INCLUDE
  ${METISX_SDK_PATH}/mu
  ${METISX_SDK_PATH}/metisx
  LIBRARY
  mu_lib_rv)
