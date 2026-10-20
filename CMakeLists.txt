cmake_minimum_required(VERSION 3.16)
project(RallyState CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

add_compile_options(-Wall -Wextra)

# ---- Core library ----
# Add new .cpp files here as you create them.
add_library(rallystate_lib
    src/scoring_rules.cpp
    src/match_state_machine.cpp
    src/glicko2.cpp
    src/replay.cpp
)
target_include_directories(rallystate_lib PUBLIC include)

# ---- Fetch Catch2 (testing framework) ----
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)
list(APPEND CMAKE_MODULE_PATH "${catch2_SOURCE_DIR}/extras")

# ---- Tests ----
# Add new test .cpp files here as you create them.
add_executable(rallystate_tests
    tests/test_scoring_rules.cpp
    tests/test_match_state_machine.cpp
    tests/test_glicko2.cpp
    tests/test_replay.cpp
    tests/test_final_set_rules.cpp
)
target_link_libraries(rallystate_tests PRIVATE rallystate_lib Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(rallystate_tests)

# ---- Command line tool ----
add_executable(rallystate_cli apps/rallystate_cli.cpp)
target_link_libraries(rallystate_cli PRIVATE rallystate_lib)
