
# ---------------------------------------- subdirectories --------------------------------------------------------------
# ======================================================================================================================



# ---------------------------------------- link libraries --------------------------------------------------------------
# ======================================================================================================================
add_subdirectory(cxxopts EXCLUDE_FROM_ALL)
target_link_libraries(${Target} PRIVATE cxxopts)

add_subdirectory(cxxshm EXCLUDE_FROM_ALL)
target_link_libraries(${Target} PRIVATE cxxshm)

add_subdirectory(cxxendian)
target_link_libraries(${Target} PRIVATE cxxendian)

target_link_libraries(${Target} PRIVATE rt)
