cmake_minimum_required(VERSION 3.20)

project(
    COLA_JVM
    VERSION 0.1.0
    LANGUAGES CXX Java
)

if (WIN32)
    message(FATAL_ERROR "COLA_JVM does not support Windows")
endif ()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Homebrew clang-tidy does not inherit AppleClang's implicit SDK lookup. Recording
# the SDK explicitly keeps the generated compilation database usable by both.
if (APPLE AND NOT CMAKE_OSX_SYSROOT)
    execute_process(
        COMMAND xcrun --sdk macosx --show-sdk-path
        OUTPUT_VARIABLE CMAKE_OSX_SYSROOT
        OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY
    )
endif ()

include(FetchContent)
include(GNUInstallDirs)

find_package(COLA REQUIRED)
find_package(JNI REQUIRED)
find_package(Java 17 REQUIRED COMPONENTS Development)

set(COLA_MODULE_NAME COLA_JVM)
option(BUILD_WITH_SWIG "Regenerate the checked-in Java/C++ wrappers from cola.i" OFF)

FetchContent_Declare(
    jni_bind
    GIT_REPOSITORY https://github.com/google/jni-bind.git
    GIT_TAG Release-1.5.1
    GIT_SHALLOW TRUE
    # JNI Bind is header-only. Do not enter its test-oriented CMake project,
    # which would unnecessarily fetch GoogleTest.
    SOURCE_SUBDIR cola-jvm-header-only
)
FetchContent_MakeAvailable(jni_bind)

set(API_CLASSES_DIR "${CMAKE_CURRENT_BINARY_DIR}/api-classes")
set(API_JAR "${CMAKE_CURRENT_BINARY_DIR}/cola-jvm-api.jar")
file(GLOB_RECURSE API_SOURCES CONFIGURE_DEPENDS
     "${CMAKE_CURRENT_SOURCE_DIR}/api/src/main/java/*.java"
)

file(GLOB NATIVE_SOURCES CONFIGURE_DEPENDS COLA_JVM/*.cc)
if (BUILD_WITH_SWIG)
    find_package(SWIG 4 REQUIRED COMPONENTS java)
    include(UseSWIG)
    set(SWIG_JAVA_DIR "${CMAKE_CURRENT_BINARY_DIR}/swig_gen")
    set(CMAKE_SWIG_OUTDIR "${SWIG_JAVA_DIR}")
    set(UseSWIG_MODULE_VERSION 2)
    set_property(SOURCE cola.i PROPERTY CPLUSPLUS ON)
    set_property(SOURCE cola.i PROPERTY SWIG_MODULE_NAME cola_jvm_native)
    set_property(SOURCE cola.i PROPERTY SWIG_FLAGS -package org.cola.api)
    swig_add_library(
        ${COLA_MODULE_NAME}
        TYPE SHARED
        LANGUAGE java
        SOURCES cola.i ${NATIVE_SOURCES}
    )
    set_target_properties(${COLA_MODULE_NAME} PROPERTIES SWIG_USE_TARGET_INCLUDE_DIRECTORIES TRUE)
    add_custom_command(
        TARGET ${COLA_MODULE_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${SWIG_JAVA_DIR}"
                "${CMAKE_CURRENT_SOURCE_DIR}/swig_gen"
        COMMENT "Updating the checked-in SWIG wrappers"
    )
else ()
    set(SWIG_JAVA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/swig_gen")
    add_library(${COLA_MODULE_NAME} SHARED ${NATIVE_SOURCES} "${SWIG_JAVA_DIR}/colaJAVA_wrap.cxx")
endif ()

file(GLOB SWIG_JAVA_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/swig_gen/*.java")

target_compile_features(${COLA_MODULE_NAME} PRIVATE cxx_std_20)
target_include_directories(
    ${COLA_MODULE_NAME} PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                               $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_include_directories(
    ${COLA_MODULE_NAME} SYSTEM PRIVATE ${JNI_INCLUDE_DIRS} "${jni_bind_SOURCE_DIR}"
)
target_link_libraries(
    ${COLA_MODULE_NAME}
    PUBLIC COLA
    PRIVATE ${JAVA_JVM_LIBRARY} ${CMAKE_DL_LIBS}
)
set_target_properties(
    ${COLA_MODULE_NAME}
    PROPERTIES VERSION "${PROJECT_VERSION}"
               SOVERSION "${PROJECT_VERSION_MAJOR}"
               SUFFIX "${CMAKE_SHARED_LIBRARY_SUFFIX}"
               CXX_CLANG_TIDY "${COLA_CMAKE_CXX_CLANG_TIDY}"
               BUILD_WITH_INSTALL_RPATH TRUE
               INSTALL_RPATH_USE_LINK_PATH TRUE
)

add_custom_command(
    OUTPUT "${API_JAR}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${API_CLASSES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${API_CLASSES_DIR}"
    COMMAND ${Java_JAVAC_EXECUTABLE} --release 17 -d "${API_CLASSES_DIR}" ${API_SOURCES}
            ${SWIG_JAVA_SOURCES}
    COMMAND ${Java_JAR_EXECUTABLE} --create --file "${API_JAR}" -C "${API_CLASSES_DIR}" .
    DEPENDS ${COLA_MODULE_NAME} ${API_SOURCES} ${SWIG_JAVA_SOURCES}
    COMMENT "Building zero-copy cola-jvm-api.jar"
    COMMAND_EXPAND_LISTS VERBATIM
)
add_custom_target(cola-jvm-api ALL DEPENDS "${API_JAR}")

install(TARGETS ${COLA_MODULE_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${COLA_MODULE_NAME})
install(FILES "${API_JAR}" DESTINATION ${CMAKE_INSTALL_LIBDIR}/${COLA_MODULE_NAME})
install(FILES COLA_JVM/JvmFilters.hh DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${COLA_MODULE_NAME})

enable_testing()
if (BUILD_TESTING)
    add_subdirectory(tests)
endif ()

setup_cola_quality_tools(EXCLUDE_PATTERNS ".*/build[^/]*/.*" ".*/swig_gen/.*")
