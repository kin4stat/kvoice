cmake_minimum_required (VERSION 3.15)

project ("kvoice")

option(BUILD_KVOICE_EXAMPLES "Build the examples" OFF)
option(KVOICE_BUILD_STATIC "Build static libs" ON)

find_package(Opus CONFIG REQUIRED)

set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(HPP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/kvoice")

add_library(kvoice	  "${SRC_DIR}/kvoice.cpp"
					  "${HPP_DIR}/kvoice.hpp" 
					  "${SRC_DIR}/sound_output_impl.hpp" 
					  "${SRC_DIR}/sound_output_impl.cpp" 
					  "${HPP_DIR}/sound_output.hpp"
					  "${HPP_DIR}/sound_input.hpp"
					  "${SRC_DIR}/sound_input_impl.hpp" 
					  "${SRC_DIR}/sound_input_impl.cpp" 
					  "${HPP_DIR}/kv_vector.hpp" 
					  "${HPP_DIR}/voice_exception.hpp"
				      "${HPP_DIR}/stream.hpp" 
					  "${SRC_DIR}/stream_impl.hpp" "${SRC_DIR}/stream_impl.cpp" "${SRC_DIR}/ringbuffer.hpp")

add_library(kin4stat::kvoice ALIAS kvoice)

target_compile_features(kvoice PUBLIC cxx_std_20)
target_include_directories(kvoice PRIVATE ${HPP_DIR} PUBLIC ${INCLUDE_DIR})

target_compile_definitions(${PROJECT_NAME} PRIVATE EXPORT_KVOICE_API)

if (${KVOICE_BUILD_STATIC})
	target_compile_definitions(${PROJECT_NAME} PUBLIC KVOICE_STATIC)
endif()

target_link_libraries(kvoice PUBLIC Opus::opus OpenAL::OpenAL PRIVATE kin4stat::ktsignal)

if (${BUILD_KVOICE_EXAMPLES}) 
	add_subdirectory("examples")
endif()