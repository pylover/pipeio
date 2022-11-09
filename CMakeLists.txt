cmake_minimum_required(VERSION 3.7)
project(pipeio 
    VERSION 1.0.0
    LANGUAGES C
)


configure_file(manifest.h.in manifest.h)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE=1")


include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_BINARY_DIR}
)


# Static Library
add_library(pipeio STATIC ev.c pipeio.c)


# Install
install(TARGETS pipeio DESTINATION "lib")
install(FILES pipeio.h DESTINATION "include")


# CPack
set(CPACK_PACKAGE_FILE_NAME "libpipeio-${PROJECT_VERSION}")
set(CPACK_SET_DESTDIR true)
set(CPACK_PACKAGE_NAME libpipeio)
set(CPACK_PACKAGE_CONTACT "Vahid Mardani <vahid.mardani@gmail.com>")
set(CPACK_GENERATOR DEB)
include(CPack)


# Testing
include(CTest)
list(APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure")


# Mockup
add_executable(mockup mockup.c)
target_link_libraries(mockup PUBLIC mrb clog)


# Test Magic ringbuffer
add_executable(pipeio_test pipeio_test.c)
target_link_libraries(pipeio_test PUBLIC pipeio mrb clog)
add_test(NAME pipeio_test COMMAND pipeio_test)
