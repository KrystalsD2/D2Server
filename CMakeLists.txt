cmake_minimum_required(VERSION 3.16)
project(D2Server100Reconstructed VERSION 0.4 LANGUAGES CXX)
find_package(Threads REQUIRED)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CORE_SOURCES
    src/PlayerToken.cpp
    src/D2GameBindings.cpp
    src/D2NetApi100.cpp
    src/EngineGlobals100.cpp
    src/FogApi100.cpp
    src/FogTransport100.cpp
    src/RealmConfig100.cpp
    src/GameWorker100.cpp
    src/ServerRuntime100.cpp
    src/Diagnostics100.cpp
    src/StormApi100.cpp
    src/GameDataHash100.cpp
    src/Callbacks100.cpp
    src/Startup100.cpp
    src/CORE/FromMCP.cpp
    src/CORE/ToMCP.cpp
    src/CORE/CharServ.cpp
    src/QueryInterface.cpp)
add_library(d2server100_core STATIC ${CORE_SOURCES})
target_include_directories(d2server100_core PUBLIC include)
target_link_libraries(d2server100_core PUBLIC Threads::Threads)

add_executable(token_tests tests/token_tests.cpp)
target_link_libraries(token_tests PRIVATE d2server100_core)
add_executable(layout_tests tests/layout_tests.cpp)
target_link_libraries(layout_tests PRIVATE d2server100_core)
add_executable(bridge_tests tests/bridge_tests.cpp)
target_link_libraries(bridge_tests PRIVATE d2server100_core)
add_executable(realm_config_tests tests/realm_config_tests.cpp)
target_link_libraries(realm_config_tests PRIVATE d2server100_core)
add_executable(worker_tests tests/worker_tests.cpp)
target_link_libraries(worker_tests PRIVATE d2server100_core)
add_executable(transport_tests tests/transport_tests.cpp)
target_link_libraries(transport_tests PRIVATE d2server100_core)
add_executable(d2net_startup_tests tests/d2net_startup_tests.cpp)
target_link_libraries(d2net_startup_tests PRIVATE d2server100_core)
add_executable(teardown_tests tests/teardown_tests.cpp)
target_link_libraries(teardown_tests PRIVATE d2server100_core)
add_executable(engine_globals_tests tests/engine_globals_tests.cpp)
target_link_libraries(engine_globals_tests PRIVATE d2server100_core)
add_executable(startup_phase_tests tests/startup_phase_tests.cpp)
target_link_libraries(startup_phase_tests PRIVATE d2server100_core)

enable_testing()
add_test(NAME player_tokens COMMAND token_tests)
add_test(NAME layouts COMMAND layout_tests)
add_test(NAME bridges COMMAND bridge_tests)
add_test(NAME realm_config COMMAND realm_config_tests)
add_test(NAME worker_loop COMMAND worker_tests)
add_test(NAME fog_transport COMMAND transport_tests)
add_test(NAME d2net_startup COMMAND d2net_startup_tests)
add_test(NAME teardown_order COMMAND teardown_tests)
add_test(NAME engine_globals COMMAND engine_globals_tests)
add_test(NAME startup_phases COMMAND startup_phase_tests)

if(WIN32)
    add_library(D2Server SHARED ${CORE_SOURCES} src/DllMain.cpp src/Exports.cpp D2Server100.def)
    target_include_directories(D2Server PRIVATE include)
    target_compile_definitions(D2Server PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_link_libraries(D2Server PRIVATE Threads::Threads ws2_32)
    set_target_properties(D2Server PROPERTIES OUTPUT_NAME "D2Server")
endif()
