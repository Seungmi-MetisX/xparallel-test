cmake_minimum_required(VERSION 3.26)

# test executable
project(pgxvectors_offloading_test)
add_executable(${PROJECT_NAME} inverted_mu_offload_test.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE /home/smshin/workspace/sdk/metisx)
target_link_directories(${PROJECT_NAME} PRIVATE /home/smshin/workspace/sdk/out/build/Debug/metisx)
target_link_libraries(${PROJECT_NAME} metisx)
target_compile_definitions(${PROJECT_NAME} PRIVATE _SIM_=1)

add_subdirectory(helper)
add_subdirectory(xparallel)