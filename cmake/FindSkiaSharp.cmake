# FindSkiaSharp.cmake
# Find the SkiaSharp library
#
# This module defines:
#  SkiaSharp_FOUND        - True if SkiaSharp is found
#  SkiaSharp_INCLUDE_DIRS - The SkiaSharp include directories
#  SkiaSharp_LIBRARIES    - The SkiaSharp libraries
#  SkiaSharp_VERSION      - The SkiaSharp version string

# Try to find SkiaSharp using standard paths
find_path(SkiaSharp_INCLUDE_DIR
    NAMES SkiaSharp.h
    PATHS
        ${SkiaSharp_DIR}/include
        $ENV{SkiaSharp_DIR}/include
        ${SKIASHARP_ROOT}/include
        $ENV{SKIASHARP_ROOT}/include
        /usr/include
        /usr/local/include
        /opt/skiasharp/include
        c:/skiasharp/include
    PATH_SUFFIXES SkiaSharp skiasharp
)

# Find library in standard locations
find_library(SkiaSharp_LIBRARY
    NAMES SkiaSharp skiasharp libSkiaSharp
    PATHS
        ${SkiaSharp_DIR}/lib
        $ENV{SkiaSharp_DIR}/lib
        ${SKIASHARP_ROOT}/lib
        $ENV{SKIASHARP_ROOT}/lib
        /usr/lib
        /usr/local/lib
        /opt/skiasharp/lib
        c:/skiasharp/lib
)

# Handle NuGet package installations
if(WIN32 AND NOT SkiaSharp_LIBRARY)
    # Look for NuGet packages
    find_path(SkiaSharp_NUGET_DIR
        NAMES SkiaSharp.dll
        PATHS
            ${CMAKE_BINARY_DIR}/packages/SkiaSharp*
            ${CMAKE_SOURCE_DIR}/packages/SkiaSharp*
            ~/.nuget/packages/SkiaSharp*
            $ENV{USERPROFILE}/.nuget/packages/SkiaSharp*
        PATH_SUFFIXES lib/net462 lib/netstandard2.0 runtimes/win-x64/native runtimes/win-x86/native
    )
    
    if(SkiaSharp_NUGET_DIR)
        # Set library from NuGet
        find_file(SkiaSharp_LIBRARY
            NAMES SkiaSharp.dll
            PATHS ${SkiaSharp_NUGET_DIR}
            NO_DEFAULT_PATH
        )
        
        # Attempt to find include dir
        find_path(SkiaSharp_INCLUDE_DIR
            NAMES SkiaSharp.h
            PATHS ${SkiaSharp_NUGET_DIR}/../../
            PATH_SUFFIXES build/native/include
            NO_DEFAULT_PATH
        )
    endif()
endif()

# Try to get version information
if(SkiaSharp_INCLUDE_DIR)
    if(EXISTS "${SkiaSharp_INCLUDE_DIR}/SkiaSharpVersion.h")
        file(STRINGS "${SkiaSharp_INCLUDE_DIR}/SkiaSharpVersion.h" SkiaSharp_VERSION_LINE REGEX "^#define[ \t]+SKIASHARP_VERSION[ \t]+\"[0-9.]+\"")
        if(SkiaSharp_VERSION_LINE)
            string(REGEX REPLACE "^#define[ \t]+SKIASHARP_VERSION[ \t]+\"([0-9.]+)\"" "\\1" SkiaSharp_VERSION "${SkiaSharp_VERSION_LINE}")
        endif()
    endif()
endif()

# Standard find package handling
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SkiaSharp
    REQUIRED_VARS SkiaSharp_LIBRARY SkiaSharp_INCLUDE_DIR
    VERSION_VAR SkiaSharp_VERSION
)

# Set output variables
if(SkiaSharp_FOUND)
    set(SkiaSharp_LIBRARIES ${SkiaSharp_LIBRARY})
    set(SkiaSharp_INCLUDE_DIRS ${SkiaSharp_INCLUDE_DIR})
    
    # Mark as advanced
    mark_as_advanced(SkiaSharp_INCLUDE_DIR SkiaSharp_LIBRARY)
endif()
