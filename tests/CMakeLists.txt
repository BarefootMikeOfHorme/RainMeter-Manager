# Tests CMakeLists.txt for RainmeterManager

# Include GoogleTest or other testing framework
include(CTest)

# Set test include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/ui
    ${CMAKE_SOURCE_DIR}/src/widgets
)

# Define test source directories
set(TEST_DIRS
    unit
    integration
)

# Add test subdirectories if they contain CMakeLists.txt files
foreach(DIR ${TEST_DIRS})
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/CMakeLists.txt")
        add_subdirectory(${DIR})
    endif()
endforeach()

# If no test subdirectories are found, create a placeholder test to avoid build errors
if(NOT TARGET unit_tests AND NOT TARGET integration_tests)
    # Create an empty test target to prevent build errors
    add_custom_target(placeholder_tests
        COMMENT "No tests configured yet. Add tests to unit/ or integration/ directories."
    )
    
    # Placeholder test file for demonstration
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/placeholder_test.cpp
        "#include <iostream>\nint main() { std::cout << \"No tests configured yet.\\n\"; return 0; }\n"
    )
    
    # Build the placeholder executable
    add_executable(placeholder_test ${CMAKE_CURRENT_BINARY_DIR}/placeholder_test.cpp)
    add_test(NAME placeholder COMMAND placeholder_test)
endif()

message(STATUS "Configured tests for ${PROJECT_NAME}")
