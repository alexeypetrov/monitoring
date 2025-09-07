cmake_minimum_required(VERSION 3.12)

set(PATCH_VERSION "1" CACHE INTERNAL "Patch version")
set(PROJECT_VERSION 0.0.${PATCH_VERSION})

project(monitoring VERSION ${PROJECT_VERSION})

find_package(Boost COMPONENTS program_options system REQUIRED)
find_package(CURL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SQLITE3 REQUIRED sqlite3)

add_executable(monitoring
    main.cpp
)

target_link_libraries(monitoring PRIVATE
    Boost::system
    Boost::program_options
    CURL::libcurl
    ${SQLITE3_LIBRARIES}
)

set_target_properties(monitoring PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

target_include_directories(monitoring
    PRIVATE "${CMAKE_BINARY_DIR}"
    PRIVATE ${SQLITE3_INCLUDE_DIRS}
)

if (MSVC)
    target_compile_options(monitoring PRIVATE
        /W4
    )
else ()
    target_compile_options(monitoring PRIVATE
        -Wall -Wextra -pedantic -Werror
    )
endif()

install(TARGETS monitoring RUNTIME DESTINATION bin)

set(CPACK_GENERATOR DEB)
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_CONTACT alexeypetrov@hotmail.com)
include(CPack)
