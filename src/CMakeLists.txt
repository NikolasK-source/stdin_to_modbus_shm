# ---------------------------------------- source files (*.cpp, *.cc, ...) ---------------------------------------------
# ======================================================================================================================

target_sources(${Target} PRIVATE main.cpp)
target_sources(${Target} PRIVATE license.cpp)
target_sources(${Target} PRIVATE InputParser.cpp)


# ---------------------------------------- header files (*.jpp, *.h, ...) ----------------------------------------------
# ======================================================================================================================

target_sources(${Target} PRIVATE input_parse.hpp)
target_sources(${Target} PRIVATE split_string.hpp)
target_sources(${Target} PRIVATE license.hpp)
target_sources(${Target} PRIVATE InputParser.hpp)
target_sources(${Target} PRIVATE InputParser_float.hpp)
target_sources(${Target} PRIVATE InputParser_int.hpp)


# ---------------------------------------- subdirectories --------------------------------------------------------------
# ======================================================================================================================
