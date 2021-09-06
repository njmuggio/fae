message(STATUS "Including fae tests")

set(FAE_CATCH_VERSION 2.13.7)

add_subdirectory(Catch2-${FAE_CATCH_VERSION})


add_executable(testFae)
target_sources(testFae PRIVATE
  main.cpp
  testLibrary.cpp
  testTemplate.cpp
)
target_link_libraries(testFae PUBLIC Catch2::Catch2 fae::fae)

include(Catch2-${FAE_CATCH_VERSION}/contrib/Catch.cmake)
catch_discover_tests(testFae)
