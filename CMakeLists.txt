cmake_minimum_required(VERSION 3.24)
include(FetchContent)
set(CMAKE_CXX_STANDARD 20)

##########################################################
# Global project setup
##########################################################
message("Setting up project...")
project(use)
# Including headers.
include_directories(include)
# Including sources.
file(GLOB projectSources main.cpp src/* src/**/*)
message("Included source files: ${projectSources}")

##########################################################
# Dependencies setup
##########################################################
# Download Elements library.
FetchContent_Declare(
        dep_elements
        GIT_REPOSITORY https://github.com/andreondra/elements.git
        GIT_PROGRESS TRUE
        GIT_TAG master
)
FetchContent_MakeAvailable(dep_elements)

imgui_bundle_add_app(use ${projectSources})