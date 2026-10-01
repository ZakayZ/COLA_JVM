set(TEST_CLASSES_DIR "${CMAKE_CURRENT_BINARY_DIR}/java-classes")
set(TEST_JAR "${CMAKE_CURRENT_BINARY_DIR}/java-scaler.jar")
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/java/*.java")
list(APPEND TEST_SOURCES
     "${PROJECT_SOURCE_DIR}/examples/java-scaler/src/main/java/org/cola/example/ScaleConverter.java"
)

add_custom_command(
    OUTPUT "${TEST_JAR}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${TEST_CLASSES_DIR}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${TEST_CLASSES_DIR}"
    COMMAND ${Java_JAVAC_EXECUTABLE} --release 17 -cp "${API_JAR}" -d "${TEST_CLASSES_DIR}"
            ${TEST_SOURCES}
    COMMAND ${Java_JAR_EXECUTABLE} --create --file "${TEST_JAR}" -C "${TEST_CLASSES_DIR}" .
    DEPENDS ${TEST_SOURCES} "${API_JAR}"
    VERBATIM
)
add_custom_target(cola-jvm-test-module ALL DEPENDS "${TEST_JAR}")
add_dependencies(cola-jvm-test-module cola-jvm-api)

add_cola_jvm_library(
    COLA_JVM_TestModule
    NO_INSTALL
    JAR
    "${TEST_JAR}"
    DEPENDS
    cola-jvm-test-module
    GENERATORS
    "test_generator=org.cola.test.TestGenerator"
    CONVERTERS
    "scale_converter=org.cola.example.ScaleConverter"
    "replacement_converter=org.cola.test.ReplacementConverter"
    UNSAFE_GENERATORS
    "test_unsafe_generator=org.cola.test.TestUnsafeGenerator"
    UNSAFE_CONVERTERS
    "test_unsafe_converter=org.cola.test.TestUnsafeConverter"
    WRITERS
    "test_writer=org.cola.test.TestWriter"
)

add_executable(cola-jvm-filters-test jvm_filters_test.cc)
target_compile_features(cola-jvm-filters-test PRIVATE cxx_std_20)
target_link_libraries(cola-jvm-filters-test PRIVATE ${COLA_MODULE_NAME})
set_target_properties(
    cola-jvm-filters-test PROPERTIES CXX_CLANG_TIDY "${COLA_CMAKE_CXX_CLANG_TIDY}"
)

add_executable(cola-jvm-generated-module-test generated_module_test.cc)
target_compile_features(cola-jvm-generated-module-test PRIVATE cxx_std_20)
target_link_libraries(cola-jvm-generated-module-test PRIVATE COLA)
set_target_properties(
    cola-jvm-generated-module-test PROPERTIES CXX_CLANG_TIDY "${COLA_CMAKE_CXX_CLANG_TIDY}"
)

add_test(NAME cola-jvm-all-filter-ownership COMMAND cola-jvm-filters-test "${TEST_JAR}")
add_test(NAME cola-jvm-java-bridge
         COMMAND "${Java_JAVA_EXECUTABLE}" -cp "${API_JAR}:${TEST_JAR}" org.cola.test.BridgeTest
                 "$<TARGET_FILE:COLA_JVM>" "${TEST_JAR}"
)
add_test(NAME cola-jvm-generated-module COMMAND cola-jvm-generated-module-test
                                                "${CMAKE_CURRENT_BINARY_DIR}"
)
