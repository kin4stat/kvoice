cmake_minimum_required(VERSION 3.15)

project("kvoice-example")

add_executable(${PROJECT_NAME} "main.cpp")

target_link_libraries(${PROJECT_NAME} PRIVATE kin4stat::kvoice)