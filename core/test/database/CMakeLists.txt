cmake_minimum_required(VERSION 3.0)
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories(${CMAKE_BINARY_DIR}/include)
if (APPLE)
    add_definitions(-DGTEST_USE_OWN_TR1_TUPLE)
    add_definitions(-D__GLIBCXX__)
endif (APPLE)

include_directories(../lib/libledger-test/)

add_executable(ledger-core-database-tests main.cpp pool_tests.cpp query_filters_tests.cpp query_builder_tests.cpp
            BaseFixture.cpp BaseFixture.h IntegrationEnvironment.cpp IntegrationEnvironment.h
        database_soci_proxy_tests.cpp MemoryDatabaseProxy.cpp MemoryDatabaseProxy.h sqlcipher_tests.cpp)

target_link_libraries(ledger-core-database-tests gtest gtest_main)
target_link_libraries(ledger-core-database-tests ledger-core-static)
target_link_libraries(ledger-core-database-tests ledger-test)
target_include_directories(ledger-core-database-tests PUBLIC ../../../core/src/)

include(CopyAndInstallImportedTargets)
if (SYS_OPENSSL)
    find_package(OpenSSL)
    if(WIN32)
        target_link_libraries( ledger-core-database-tests ws2_32 crypt32 )
    endif()
    copy_install_imported_targets(ledger-core-database-tests OpenSSL::Crypto uv)
else()
    copy_install_imported_targets(ledger-core-database-tests crypto uv)
endif()
add_test(NAME ledger-core-database-tests COMMAND ledger-core-database-tests)