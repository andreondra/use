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
file(GLOB projectSources src/* src/**/*)
message("Included source files: ${projectSources}")

##########################################################
# Dependencies setup
##########################################################
# Download imgui_bundle
Set(FETCHCONTENT_QUIET FALSE)
FetchContent_Declare(
        imgui_bundle
        GIT_REPOSITORY https://github.com/pthom/imgui_bundle.git
        GIT_PROGRESS TRUE
        # Enter the desired git tag below
        GIT_TAG main
)
FetchContent_MakeAvailable(imgui_bundle)
# Make cmake function `imgui_bundle_add_app` available
list(APPEND CMAKE_MODULE_PATH ${IMGUIBUNDLE_CMAKE_PATH})
include(imgui_bundle_add_app)

# Download imgui memory editor.
FetchContent_Declare(
        imgui_memedit
        GIT_REPOSITORY https://github.com/ocornut/imgui_club.git
        GIT_PROGRESS TRUE
        GIT_TAG main
)
FetchContent_MakeAvailable(imgui_memedit)
include_directories(${imgui_memedit_SOURCE_DIR}/imgui_memory_editor)

# Download googletest
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_PROGRESS TRUE
        GIT_TAG main
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
enable_testing()

##########################################################
# Build setup
##########################################################
imgui_bundle_add_app(use main.cpp ${projectSources})

##########################################################
# Tests setup
##########################################################
file(COPY tests/testfiles DESTINATION .)
file(GLOB unitTests tests/unit/* tests/integration/*)
imgui_bundle_add_app(UnitTests ${unitTests} ${projectSources})
target_link_libraries(UnitTests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(UnitTests)