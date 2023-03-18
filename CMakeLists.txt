cmake_minimum_required(VERSION 3.24)
include(FetchContent)
set(CMAKE_CXX_STANDARD 20)

##########################################################
# Global project setup
##########################################################
message("Setting up project...")
project(use)

# Project descriptions =====================================================
set(PROJECT_FULL_NAME "Universal System Emulator")
set(PROJECT_FULL_COPYRIGHT "2023 Ondrej Golasowski")
set(PROJECT_FULL_AUTHOR "Ondrej Golasowski")

# Project directories ======================================================
set(PROJECT_INCLUDE_DIR include)
set(DOCS_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/docs)
# Warning: following directory will be removed with CMake 'clean' command!
set(DOCS_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/docs/_build)
set(DOCS_DOXYGEN_SUBDIRECTORY docs_doxygen)
set(DOCS_SPHINX_SUBDIRECTORY docs_sphinx)

# Docs generators' config files ============================================
set(DOCS_CONF_DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
set(DOCS_CONF_DOXYGEN_OUT ${CMAKE_CURRENT_SOURCE_DIR}/docs/_build/Doxyfile)
set(DOCS_CONF_SPHINX_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/conf.in.py)
set(DOCS_CONF_SPHINX_OUT ${CMAKE_CURRENT_SOURCE_DIR}/docs/_build/conf.py)

# Include additional modules.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_src")

# Including headers.
include_directories(${PROJECT_INCLUDE_DIR})

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

# Download ImGuiFileDialog
FetchContent_Declare(
        imgui_filedialog
        GIT_REPOSITORY https://github.com/aiekick/ImGuiFileDialog
        GIT_PROGRESS TRUE
        GIT_TAG Lib_Only
)
FetchContent_MakeAvailable(imgui_filedialog)
include_directories(${imgui_filedialog_SOURCE_DIR})

##########################################################
# Build setup
##########################################################
imgui_bundle_add_app(use main.cpp ${projectSources})

##########################################################
# Tests setup
##########################################################
# Download test binaries
message("Downloading 6502 test binaries...")
file(
        DOWNLOAD
        https://github.com/andreondra/use-tests-6502-65C02/releases/download/latest/6502_functional_test.bin
        ${CMAKE_CURRENT_BINARY_DIR}/testfiles/6502_functional_test.bin
        SHOW_PROGRESS
)
file(
        DOWNLOAD
        https://github.com/andreondra/use-tests-6502-65C02/releases/download/latest/6502_interrupt_test.bin
        ${CMAKE_CURRENT_BINARY_DIR}/testfiles/6502_interrupt_test.bin
        SHOW_PROGRESS
)

# Copy test assets to build.
file(COPY tests/testfiles DESTINATION .)

file(GLOB unitTests tests/unit/*)
imgui_bundle_add_app(UnitTests ${unitTests} ${projectSources})
target_link_libraries(UnitTests PRIVATE GTest::gtest_main)

file(GLOB integrationTests tests/integration/*)
imgui_bundle_add_app(IntegrationTests ${integrationTests} ${projectSources})
target_link_libraries(IntegrationTests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(UnitTests)
gtest_discover_tests(IntegrationTests)

##########################################################
# Docs generator setup
##########################################################
find_package(Doxygen)
if (DOXYGEN_FOUND)

    # Doxygen ============================================================
    message("[Doxygen] Configuring...")
    # Make docs build directory
    file(MAKE_DIRECTORY ${DOCS_OUTPUT_DIR})
    # Request to configure the file.
    # This will replace all @VARIABLES@ with the ones configured in CMake. See global project conf at the top.
    configure_file(${DOCS_CONF_DOXYGEN_IN} ${DOCS_CONF_DOXYGEN_OUT} @ONLY)

    message("[Doxygen] Adding Doxygen target...")
    # Note: do not put "ALL" - this builds docs together with application EVERY TIME!
    add_custom_target(docs
            COMMAND ${DOXYGEN_EXECUTABLE} ${DOCS_CONF_DOXYGEN_OUT}
            WORKING_DIRECTORY ${DOCS_OUTPUT_DIR}
            COMMENT "Generating API documentation with Doxygen..."
            VERBATIM)

    # Add docs output directory to CMake 'clean' target.
    set_property(
            TARGET docs
            APPEND
            PROPERTY ADDITIONAL_CLEAN_FILES ${DOCS_OUTPUT_DIR} ${DOCS_SOURCE_DIR}/_exhale
    )
    message("[Doxygen] Set up successfully!")

    # ReadTheDocs ============================================================
    # Set Python packages to check
    set(REQUIRED_PYTHON_PACKAGES "breathe" "exhale" "sphinx-rtd-theme" "sphinx-sitemap")
    set(PYTHON_PACKAGE_MISSING FALSE)

    # Find Python and Sphinx
    message("[ReadTheDocs] Looking for Python3 and Sphinx")
    find_package (Python3 COMPONENTS Interpreter)
    find_package(Sphinx)
    if(Python3_FOUND AND Sphinx_FOUND)

        # Find pip
        message("[ReadTheDocs] Looking for pip")
        execute_process(
                COMMAND ${Python3_EXECUTABLE} -m pip
                RESULT_VARIABLE EXIT_CODE
                OUTPUT_QUIET
        )

        if (NOT ${EXIT_CODE} EQUAL 0)
            message(WARNING "[ReadTheDocs] pip not installed, can't automatically check dependencies.")
        else()
            # Check if all the required packages are installed with pip
            message("[ReadTheDocs] Looking for required Python packages")
            foreach(PACKAGE IN LISTS REQUIRED_PYTHON_PACKAGES)
                execute_process(
                        COMMAND ${Python3_EXECUTABLE} -m pip show ${PACKAGE}
                        RESULT_VARIABLE EXIT_CODE
                        OUTPUT_QUIET
                )
                if (NOT ${EXIT_CODE} EQUAL 0)
                    message(WARNING "[ReadTheDocs] Python package ${PACKAGE} not found, terminating.")
                    set(PYTHON_PACKAGE_MISSING TRUE)
                    break()
                endif()
            endforeach()
        endif()

        # Configure RTD target
        if(NOT PYTHON_PACKAGE_MISSING)
            message("[ReadTheDocs] Configuring target...")

            # This will replace all @VARIABLES@ with the ones configured in CMake. See global project conf at the top.
            configure_file(${DOCS_CONF_SPHINX_IN} ${DOCS_CONF_SPHINX_OUT} @ONLY)

            add_custom_target(sphinxdocs
                    COMMAND ${SPHINX_EXECUTABLE} -b html -c ${DOCS_OUTPUT_DIR} . ${DOCS_OUTPUT_DIR}/${DOCS_SPHINX_SUBDIRECTORY}
                    WORKING_DIRECTORY ${DOCS_SOURCE_DIR}
                    COMMENT "Generating API documentation with Sphinx using RTD theme..."
                    VERBATIM
                    DEPENDS docs)
        else()
            message(WARNING "[ReadTheDocs] Couldn't find some required packages. RTD won't be available.")
        endif()
    else()
        message(WARNING "[ReadTheDocs] Python3 and Sphinx needs to be installed. RTD won't be available.")
    endif()

else (DOXYGEN_FOUND)
    message(WARNING "[Doxygen] Doxygen not found, can't generate docs.")
endif (DOXYGEN_FOUND)