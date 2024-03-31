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

add_subdirectory(cxxendian)

# ---------------------------------------- link libraries --------------------------------------------------------------
# ======================================================================================================================
target_link_libraries(${Target} PRIVATE rt)
target_link_libraries(${Target} PRIVATE INTERFACE cxxopts)
target_link_libraries(${Target} PRIVATE cxxshm)
target_link_libraries(${Target} PRIVATE cxxendian)
target_link_libraries(${Target} PRIVATE ${readline_library})
target_link_libraries(${Target} PRIVATE cxxsemaphore)
