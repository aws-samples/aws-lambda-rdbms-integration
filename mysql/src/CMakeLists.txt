cmake_minimum_required(VERSION 2.8)
project(awslambda_fn)

# location of mysql headers
include_directories(/usr/include/mysql)

#this will locate the aws sdk for c++ package so that we can use its targets
find_package(aws-sdk-cpp)

# shared library for our myql udf
add_library(awslambda_fn MODULE LambdaInvoke.cpp awslambda_fn.cpp)
target_link_libraries(awslambda_fn aws-cpp-sdk-lambda)

install(TARGETS awslambda_fn LIBRARY  DESTINATION /usr/lib64/mysql56/plugin)

# test function
#add_executable(awslambda_fn_test LambdaInvoke.cpp awslambda_fn.cpp testmain.cpp)
#target_link_libraries(awslambda_fn_test aws-cpp-sdk-lambda)
