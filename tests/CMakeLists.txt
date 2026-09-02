set(TEST_CLASSES_DIR "${CMAKE_CURRENT_BINARY_DIR}/java-classes")
set(TEST_JAR "${CMAKE_CURRENT_BINARY_DIR}/java-scaler.jar")
file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/java/*.java")
list(APPEND TEST_SOURCES "${PROJECT_SOURCE_DIR}/examples/java-scaler/ScaleConverter.java")

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

add_executable(cola-jvm-filters-test jvm_filters_test.cc)
target_compile_features(cola-jvm-filters-test PRIVATE cxx_std_20)
target_link_libraries(cola-jvm-filters-test PRIVATE ${COLA_MODULE_NAME})
set_target_properties(
    cola-jvm-filters-test PROPERTIES CXX_CLANG_TIDY "${COLA_CMAKE_CXX_CLANG_TIDY}"
)

add_test(NAME cola-jvm-all-filter-ownership COMMAND cola-jvm-filters-test "${TEST_JAR}")
