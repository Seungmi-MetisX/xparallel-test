include(FetchContent)
set(METISX_SDK_PATH /home/smshin/workspace/sdk)

function(metisx_fetch_external name)
  FetchContent_Declare(
    ${name}
    ${ARGN}
  )
  FetchContent_MakeAvailable(${name})
endfunction()

function(get_link_libraries OUTPUT_LIST TARGET)
    set(${OUTPUT_LIST} "")
    __get_link_libraries(${OUTPUT_LIST} ${TARGET})
    set(${OUTPUT_LIST} ${${OUTPUT_LIST}} PARENT_SCOPE)
endfunction()

function(__get_link_libraries OUTPUT_LIST TARGET)
    if(TARGET ${TARGET})
        get_target_property(IMPORTED ${TARGET} IMPORTED)
        list(APPEND VISITED_TARGETS ${TARGET})
        if (IMPORTED)
            get_target_property(LIBS ${TARGET} INTERFACE_LINK_LIBRARIES)
        else()
            get_target_property(LIBS ${TARGET} LINK_LIBRARIES)
        endif()

        foreach(LIB IN LISTS LIBS)
            if (NOT LIB IN_LIST VISITED_TARGETS)
                list(APPEND ${OUTPUT_LIST} ${LIB})
                # message(STATUS "OUTPUT_LIST=${${OUTPUT_LIST}}")
                __get_link_libraries(${OUTPUT_LIST} ${LIB})
            endif()
        endforeach()
        set(${OUTPUT_LIST} ${${OUTPUT_LIST}} PARENT_SCOPE)
    endif()
endfunction()

function(metisx_add_shared_lib project_name)
    foreach(LIB IN LISTS ARGN)
        if (NOT LIB MATCHES "metisx_.*|mu" )
            continue()
        endif()
        __metisx_add_shared_lib(${project_name} ${LIB})
    endforeach()
endfunction()

function(__metisx_add_shared_lib project_name lib_name)
    target_link_options(${project_name} PRIVATE
    "$<$<CXX_COMPILER_ID:GNU>:SHELL:LINKER:--whole-archive $<TARGET_LINKER_FILE:${lib_name}> LINKER:--no-whole-archive>"
    )
    target_link_libraries(${project_name} PRIVATE ${lib_name})
endfunction()



function(metisx_add_test name)
    # if (INSTALL_MODE STREQUAL "0")
        set(options)
        set(oneValueArgs)
        set(multiValueArgs INCLUDE LIBRARY SOURCE DEFINITIONS)
        cmake_parse_arguments(METISX_TEST "${options}" "${oneValueArgs}"
                            "${multiValueArgs}" ${ARGN} )

        set(name ${name}_test)

        add_executable(${name} ${METISX_TEST_SOURCE} ${METISX_SDK_PATH}/test/test_main.cpp)
        message(STATUS "${name} ${METISX_TEST_DEFINITIONS}")
        target_compile_definitions(${name} PRIVATE _SIM_=1 ${METISX_TEST_DEFINITIONS})
        target_link_libraries(${name} PRIVATE
            # gtest
            # fmt::fmt
            metisx
            dl
            ${METISX_TEST_LIBRARY}
        )
        target_link_options(${name} PRIVATE "LINKER:-no-as-needed")
        target_include_directories(${name} PRIVATE ${METISX_TEST_INCLUDE})
        target_include_directories(${name} PRIVATE ${METISX_SDK_PATH}/metisx)
    # endif()
endfunction()

# add_mu_library(<name>
#             [LIB|EXE]
#             [LINKER <linker>]
#             [STARTUP <startup>]
#             [ISA <isa type>]
#             [INCLUDE <include>...]
#             [LIBRARY <library>...]
#             [SOURCE <source>...])
function(add_mu_binary name)
    set(options LIB EXE)
    set(oneValueArgs LINKER STARTUP ISA)
    set(multiValueArgs INCLUDE LIBRARY SOURCE DEFINITIONS)
    cmake_parse_arguments(METISX_MU "${options}" "${oneValueArgs}"
                          "${multiValueArgs}" ${ARGN} )

    # message("METISX_MU_INCLUDE=${METISX_MU_INCLUDE}")
    # message("METISX_MU_LIBRARY=${METISX_MU_LIBRARY}")
    # message("METISX_MU_SOURCE=${METISX_MU_SOURCE}")
    # message("METISX_MU_LINKER=${METISX_MU_LINKER}")

    set(COMMAND python3 ${METISX_SDK_PATH}/mu/build_script/build.py --output ${name} --path ${CMAKE_CURRENT_LIST_DIR})
    message(STATUS ${COMMAND})

    if (${METISX_MU_LIB})
        set(COMMAND ${COMMAND} --lib)
    endif()

    if (DEFINED METISX_MU_ISA)
        set(COMMAND ${COMMAND} --isa ${METISX_MU_ISA})
    endif()

    if (DEFINED METISX_MU_LINKER)
        set(COMMAND ${COMMAND} --linker ${METISX_MU_LINKER})
    endif()

    if (DEFINED METISX_MU_STARTUP)
        set(COMMAND ${COMMAND} --startup ${METISX_MU_STARTUP})
    endif()

    if (DEFINED METISX_MU_LIBRARY)
        set(COMMAND ${COMMAND} --library)
        foreach(lib IN LISTS METISX_MU_LIBRARY)
            set(DEPENDENCIES ${DEPENDENCIES} ${lib}_mu)
            set(COMMAND ${COMMAND} ${lib})
        endforeach()
    endif()

    if (DEFINED METISX_MU_INCLUDE)
        set(COMMAND ${COMMAND} --include)
        foreach(inc IN LISTS METISX_MU_INCLUDE)
            set(COMMAND ${COMMAND} ${inc})
        endforeach()
    endif()

    if (DEFINED METISX_MU_SOURCE)
        set(COMMAND ${COMMAND} --source)
        foreach(src IN LISTS METISX_MU_SOURCE)
            set(COMMAND ${COMMAND} ${src})
        endforeach()
    endif()

    if (DEFINED METISX_MU_DEFINITIONS)
        set(COMMAND ${COMMAND} --definitions)
        foreach(def IN LISTS METISX_MU_DEFINITIONS)
            set(COMMAND ${COMMAND} ${def})
        endforeach()
    endif()


    if ((${METISX_MU_LIB}) OR (NOT ${name} MATCHES "master*|admin.*|hif*|gaia_mu*"))
        add_custom_command(
            OUTPUT BUILD_${name}
            COMMAND ${COMMAND}
            # DEPENDS ${DEPENDENCIES} mu_lib_p4 mu_lib_rv mu_lib_rv_nofp
        )
    else()
        message(STATUS "BUILD_${name} ${METISX_SDK_PATH}/mu/out/binary/${name}_binary.h to ${METISX_SDK_PATH}/metisx/firmware/bsp/mu_binary/${name}_binary.h")
        add_custom_command(
            OUTPUT BUILD_${name}
            COMMAND ${COMMAND}
            COMMAND ${CMAKE_COMMAND} -E copy ${METISX_SDK_PATH}/mu/out/binary/${name}_binary.h
                        ${METISX_SDK_PATH}/metisx/firmware/bsp/mu_binary/${name}_binary.h
            COMMAND ${CMAKE_COMMAND} -E copy ${METISX_SDK_PATH}/mu/out/binary/${name}_bin_info.hpp
                        ${METISX_SDK_PATH}/metisx/firmware/bsp/mu_binary/${name}_bin_info.hpp
            COMMAND ${CMAKE_COMMAND} -E copy ${METISX_SDK_PATH}/mu/out/binary/${name}_text.bin
                        ${METISX_SDK_PATH}/metisx/firmware/bsp/mu_binary/${name}_text.bin
            COMMAND ${CMAKE_COMMAND} -E copy ${METISX_SDK_PATH}/mu/out/binary/${name}_rodata.bin
                        ${METISX_SDK_PATH}/metisx/firmware/bsp/mu_binary/${name}_rodata.bin
            # DEPENDS ${DEPENDENCIES} mu_lib_p4 mu_lib_rv mu_lib_rv_nofp
        )
    endif()

    add_custom_target(${name}_mu ALL
        DEPENDS BUILD_${name}
        # DEPENDS mu_lib_p4 mu_lib_rv mu_lib_rv_nofp
    )

    add_library(${name} STATIC ${METISX_MU_SOURCE})
    if (${name} STRLESS "mu")
        add_dependencies(${name} ${name}_mu)
    endif()
    target_link_libraries(${name} PRIVATE ${METISX_MU_LIBRARY})
    # target_link_libraries(${name} PUBLIC fmt::fmt)
    target_compile_definitions(${name} PRIVATE _SIM_=1 ${METISX_MU_DEFINITIONS})
    target_include_directories(${name} PRIVATE ${METISX_MU_INCLUDE})

    # if (INSTALL_MODE STREQUAL "0")
        if (NOT ${METISX_MU_LIB})
            set(so_file ${name}_gcc)

            add_library(${so_file} SHARED ${METISX_MU_SOURCE})
            set_target_properties(${so_file} PROPERTIES PREFIX "")
            set_target_properties(${so_file} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${METISX_SDK_PATH}/mu/out")
            if (${so_file} STRLESS "mu")
                add_dependencies(${so_file} ${name}_mu)
            endif()
            target_link_libraries(${so_file} PRIVATE ${METISX_MU_LIBRARY})
            # target_link_libraries(${so_file} PUBLIC fmt::fmt metisx)
            target_link_libraries(${so_file} PUBLIC metisx)
            target_compile_definitions(${so_file} PRIVATE _SIM_=1 ${METISX_MU_DEFINITIONS})
            target_include_directories(${so_file} PRIVATE ${METISX_MU_INCLUDE})
        endif()
    # endif()

endfunction()

# add_mu_library(<name>
#                     [INCLUDE <include>...]
#                     [LIBRARY <library>...]
#                     [SOURCE <source>...])
function(add_mu_library_p4 name)
    add_mu_binary(${name} ISA P4 LIB ${ARGN})
endfunction()

function(add_mu_library_rv name)
    add_mu_binary(${name} ISA RISCV LIB ${ARGN})
endfunction()

function(add_mu_library_rv_nofp name)
    add_mu_binary(${name} ISA RISCV-NOFP LIB ${ARGN})
endfunction()

# add_ms_executable(<name>
#                        [INCLUDE <include>...]
#                        [LIBRARY <library>...]
#                        [SOURCE <source>...])
function(add_mu_executable_p4 name)
    add_mu_binary(${name} ISA P4 EXE ${ARGN})
endfunction()

function(add_mu_executable_rv name)
    add_mu_binary(${name} ISA RISCV EXE ${ARGN})
endfunction()

function(add_mu_executable_rv_nofp name)
    add_mu_binary(${name} ISA RISCV-NOFP EXE ${ARGN})
endfunction()