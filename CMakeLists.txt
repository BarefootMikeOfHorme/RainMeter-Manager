# Set MSVC toolkit version to v143 for Visual Studio 2022
if(MSVC)
    set(CMAKE_GENERATOR_TOOLSET "v143" CACHE STRING "Platform toolset" FORCE)
endif()

cmake_minimum_required(VERSION 3.14)

# Define project version from version.h if it exists
set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
set(BUILD_NUMBER 0)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h")
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h" VERSION_H_CONTENTS)
    foreach(LINE ${VERSION_H_CONTENTS})
        if(LINE MATCHES "#define VERSION_MAJOR ([0-9]+)")
            set(VERSION_MAJOR ${CMAKE_MATCH_1})
        elseif(LINE MATCHES "#define VERSION_MINOR ([0-9]+)")
            set(VERSION_MINOR ${CMAKE_MATCH_1})
        elseif(LINE MATCHES "#define VERSION_PATCH ([0-9]+)")
            set(VERSION_PATCH ${CMAKE_MATCH_1})
        elseif(LINE MATCHES "#define BUILD_NUMBER ([0-9]+)")
            set(BUILD_NUMBER ${CMAKE_MATCH_1})
        endif()
    endforeach()
endif()

project(RainmeterManager 
    VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}
    DESCRIPTION "Rainmeter skin and widget management application"
    LANGUAGES CXX C
)

# Global CMake settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_DEBUG_POSTFIX "d")

# Build type configuration
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type (Debug/Release/RelWithDebInfo/MinSizeRel)" FORCE)
endif()

# Output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Include custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Generate version.h file
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h"
)

# Options
option(BUILD_TESTS "Build the test suite" ON)
option(BUILD_INSTALLER "Build the installer package" ON)
option(ENABLE_LOGGING "Enable logging" ON)
option(ENABLE_SANITIZERS "Enable address sanitizer in Debug mode" OFF)

# Compiler-specific options
if(MSVC)
    # Visual Studio specific options with security hardening
    # /W4 = Warning level 4
    # /MP = Multi-processor compilation
    # /EHsc = C++ exception handling
    # /GS = Buffer security checks
    # /sdl = SDL security checks
    add_compile_options(/W4 /MP /EHsc /GS /sdl)
    
    # /Qspectre = Spectre mitigation (optional, requires VS component)
    # Install via: VS Installer > Individual Components > "MSVC v143 Spectre-mitigated libs"
    # Uncomment to enable: add_compile_options(/Qspectre)
    
    # Debug-specific options
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od /MTd /DDEBUG /D_DEBUG")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Zi /Od /MTd /DDEBUG /D_DEBUG")
    
    # Release-specific options
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /MT /DNDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /O2 /Ob2 /MT /DNDEBUG")
    
    # Security hardening linker flags
    # /DYNAMICBASE = ASLR support
    # /NXCOMPAT = DEP (Data Execution Prevention)
    # /HIGHENTROPYVA = 64-bit ASLR
    # /GUARD:CF = Control Flow Guard
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA /GUARD:CF")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA /GUARD:CF")
    
    # SafeSEH for 32-bit builds only
    if(CMAKE_SIZEOF_VOID_P EQUAL 4)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH")
    endif()
    
    # Common definitions
    add_definitions(-D_UNICODE -DUNICODE -D_WINDOWS)
else()
    # GCC/Clang options
    add_compile_options(-Wall -Wextra -Wpedantic)
    
    # Debug-specific options
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g -O0 -DDEBUG")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -g -O0 -DDEBUG")
    
    # Enable sanitizers in debug mode if requested
    if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fsanitize=address -fno-omit-frame-pointer")
        set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fsanitize=address")
    endif()
endif()

# Conditional definitions
if(ENABLE_LOGGING)
    add_definitions(-DLOGGING_ENABLED)
endif()

# Find required dependencies
find_package(SkiaSharp QUIET)
if(NOT SkiaSharp_FOUND)
    message(STATUS "SkiaSharp not found. Using fallback drawing methods.")
    add_definitions(-DNO_SKIASHARP)
endif()

# Define source files
file(GLOB_RECURSE SOURCES
    "src/app/*.cpp"
    "src/core/*.cpp"
    "src/ui/*.cpp"
    "src/widgets/*.cpp"
)

file(GLOB_RECURSE HEADERS
    "src/app/*.h"
    "src/core/*.h"
    "src/ui/*.h"
    "src/widgets/*.h"
)

# Define resource files
file(GLOB RESOURCE_FILES "resources/*")

# Windows resource file
if(WIN32)
    set(RC_FILE "${CMAKE_CURRENT_SOURCE_DIR}/resources/resource.rc")
    set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /nologo")
endif()

# Main executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${RC_FILE})

# Add version definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    VERSION_MAJOR=${VERSION_MAJOR}
    VERSION_MINOR=${VERSION_MINOR}
    VERSION_PATCH=${VERSION_PATCH}
    BUILD_NUMBER=${BUILD_NUMBER}
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/app"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/core"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ui"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/widgets"
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    # Windows-specific libraries
    $<$<PLATFORM_ID:Windows>:user32>
    $<$<PLATFORM_ID:Windows>:gdi32>
    $<$<PLATFORM_ID:Windows>:comctl32>
    $<$<PLATFORM_ID:Windows>:shell32>
    $<$<PLATFORM_ID:Windows>:shlwapi>
    $<$<PLATFORM_ID:Windows>:comdlg32>
    $<$<PLATFORM_ID:Windows>:ole32>
    $<$<PLATFORM_ID:Windows>:advapi32>
    $<$<PLATFORM_ID:Windows>:wininet>
    $<$<PLATFORM_ID:Windows>:dbghelp>
    $<$<PLATFORM_ID:Windows>:version>
    $<$<PLATFORM_ID:Windows>:bcrypt>
    $<$<PLATFORM_ID:Windows>:wintrust>
    $<$<PLATFORM_ID:Windows>:crypt32>
    
    # SkiaSharp if available
    $<$<BOOL:${SkiaSharp_FOUND}>:${SkiaSharp_LIBRARIES}>
)

# Install rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install resource tree
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/resources/" DESTINATION share/${PROJECT_NAME})
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt" DESTINATION share/${PROJECT_NAME})

# Tests configuration
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Create source groups for better IDE organization
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Header Files" FILES ${HEADERS})
source_group("Resource Files" FILES ${RESOURCE_FILES} ${RC_FILE})

message(STATUS "Configured ${PROJECT_NAME} v${PROJECT_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
