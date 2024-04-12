# ---------------------------------------- packages --------------------------------------------------------------------
# ======================================================================================================================

find_package(cxxshm REQUIRED)
find_package(cxxsemaphore REQUIRED)
find_package(cxxopts REQUIRED)

# ---------------------------------------- external libs ---------------------------------------------------------------
# ======================================================================================================================
find_library(readline_library readline)

# ---------------------------------------- subdirectories --------------------------------------------------------------
# ======================================================================================================================

set(CLANG_FORMAT_TMP ${CLANG_FORMAT})
set(CLANG_FORMAT OFF)
add_subdirectory(cxxendian)
set(CLANG_FORMAT ${CLANG_FORMAT_TMP})

# ---------------------------------------- link libraries --------------------------------------------------------------
# ======================================================================================================================
target_link_libraries(${Target} PRIVATE rt)
target_link_libraries(${Target} PRIVATE INTERFACE cxxopts)
target_link_libraries(${Target} PRIVATE cxxshm)
target_link_libraries(${Target} PRIVATE cxxendian)
target_link_libraries(${Target} PRIVATE ${readline_library})
target_link_libraries(${Target} PRIVATE cxxsemaphore)
