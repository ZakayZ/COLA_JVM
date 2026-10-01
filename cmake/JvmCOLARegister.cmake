include(GNUInstallDirs)

function (_cola_jvm_generate_factories REGISTRATIONS_OUTPUT NAMES_OUTPUT FACTORY)
    set(REGISTRATIONS "")
    set(NAMES "")

    foreach (DEFINITION IN LISTS ARGN)
        if (NOT DEFINITION MATCHES "^([^=]+)=([^=]+)$")
            message(FATAL_ERROR "JVM filter '${DEFINITION}' must use the form COLA_NAME=JAVA_CLASS")
        endif ()

        set(FILTER_NAME "${CMAKE_MATCH_1}")
        set(CLASS_NAME "${CMAKE_MATCH_2}")
        list(APPEND NAMES "${FILTER_NAME}")

        string(REPLACE "\\" "\\\\" FILTER_NAME "${FILTER_NAME}")
        string(REPLACE "\"" "\\\"" FILTER_NAME "${FILTER_NAME}")
        string(REPLACE "\\" "\\\\" CLASS_NAME "${CLASS_NAME}")
        string(REPLACE "\"" "\\\"" CLASS_NAME "${CLASS_NAME}")

        string(APPEND REGISTRATIONS "      AddFactory<cola::jvm::${FACTORY}>(factories, jar, "
               "\"${FILTER_NAME}\", \"${CLASS_NAME}\");\n"
        )
    endforeach ()

    set(${REGISTRATIONS_OUTPUT}
        "${REGISTRATIONS}"
        PARENT_SCOPE
    )
    set(${NAMES_OUTPUT}
        "${NAMES}"
        PARENT_SCOPE
    )
endfunction ()

function (add_cola_jvm_library MODULE_NAME)
    cmake_parse_arguments(
        ARG "NO_INSTALL" "JAR"
        "GENERATORS;CONVERTERS;UNSAFE_GENERATORS;UNSAFE_CONVERTERS;WRITERS;DEPENDS" ${ARGN}
    )

    if (NOT MODULE_NAME)
        message(FATAL_ERROR "add_cola_jvm_library requires a module name")
    endif ()
    if (NOT ARG_JAR)
        message(FATAL_ERROR "add_cola_jvm_library requires JAR")
    endif ()
    if (ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown add_cola_jvm_library arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif ()

    _cola_jvm_generate_factories(
        GENERATOR_REGISTRATIONS GENERATOR_NAMES GeneratedJvmGeneratorFactory ${ARG_GENERATORS}
    )
    _cola_jvm_generate_factories(
        CONVERTER_REGISTRATIONS CONVERTER_NAMES GeneratedJvmConverterFactory ${ARG_CONVERTERS}
    )
    _cola_jvm_generate_factories(
        UNSAFE_GENERATOR_REGISTRATIONS UNSAFE_GENERATOR_NAMES GeneratedJvmUnsafeGeneratorFactory
        ${ARG_UNSAFE_GENERATORS}
    )
    _cola_jvm_generate_factories(
        UNSAFE_CONVERTER_REGISTRATIONS UNSAFE_CONVERTER_NAMES GeneratedJvmUnsafeConverterFactory
        ${ARG_UNSAFE_CONVERTERS}
    )
    _cola_jvm_generate_factories(
        WRITER_REGISTRATIONS WRITER_NAMES GeneratedJvmWriterFactory ${ARG_WRITERS}
    )

    set(FILTER_NAMES ${GENERATOR_NAMES} ${CONVERTER_NAMES} ${UNSAFE_GENERATOR_NAMES}
                     ${UNSAFE_CONVERTER_NAMES} ${WRITER_NAMES}
    )
    if (NOT FILTER_NAMES)
        message(FATAL_ERROR "add_cola_jvm_library requires at least one filter")
    endif ()
    list(LENGTH FILTER_NAMES FILTER_COUNT)
    list(REMOVE_DUPLICATES FILTER_NAMES)
    list(LENGTH FILTER_NAMES UNIQUE_FILTER_COUNT)
    if (NOT FILTER_COUNT EQUAL UNIQUE_FILTER_COUNT)
        message(FATAL_ERROR "COLA JVM filter names must be unique within a module")
    endif ()

    get_filename_component(JAR "${ARG_JAR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    get_filename_component(JAR_FILENAME "${JAR}" NAME)
    set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/${MODULE_NAME}")
    set(MODULE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${MODULE_NAME}")
    string(CONCAT FACTORY_REGISTRATIONS "${GENERATOR_REGISTRATIONS}" "${CONVERTER_REGISTRATIONS}"
                  "${UNSAFE_GENERATOR_REGISTRATIONS}" "${UNSAFE_CONVERTER_REGISTRATIONS}"
                  "${WRITER_REGISTRATIONS}"
    )

    file(MAKE_DIRECTORY "${GENERATED_DIR}")
    configure_file(
        "${COLA_JVM_TEMPLATES_DIR}/cpp/JvmModule.cc.in" "${GENERATED_DIR}/${MODULE_NAME}Module.cc"
        @ONLY
    )

    add_library(${MODULE_NAME} SHARED "${GENERATED_DIR}/${MODULE_NAME}Module.cc")
    target_compile_features(${MODULE_NAME} PRIVATE cxx_std_20)
    target_link_libraries(${MODULE_NAME} PRIVATE COLA_JVM::COLA_JVM ${CMAKE_DL_LIBS})

    if (APPLE)
        set(MODULE_INSTALL_RPATH "@loader_path/../COLA_JVM;@loader_path/..")
    else ()
        set(MODULE_INSTALL_RPATH "$ORIGIN/../COLA_JVM;$ORIGIN/..")
    endif ()
    set_target_properties(
        ${MODULE_NAME}
        PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${MODULE_OUTPUT_DIR}"
                   INSTALL_RPATH "${MODULE_INSTALL_RPATH}"
                   INSTALL_RPATH_USE_LINK_PATH TRUE
                   CXX_CLANG_TIDY "${COLA_CMAKE_CXX_CLANG_TIDY}"
    )

    set(STAGED_JAR "${MODULE_OUTPUT_DIR}/${JAR_FILENAME}")
    add_custom_command(
        OUTPUT "${STAGED_JAR}"
        COMMAND ${CMAKE_COMMAND} -E make_directory "${MODULE_OUTPUT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${JAR}" "${STAGED_JAR}"
        DEPENDS "${JAR}" ${ARG_DEPENDS}
        VERBATIM
    )
    add_custom_target(${MODULE_NAME}-jar ALL DEPENDS "${STAGED_JAR}")
    add_dependencies(${MODULE_NAME} ${MODULE_NAME}-jar)

    if (NOT ARG_NO_INSTALL)
        install(TARGETS ${MODULE_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${MODULE_NAME})
        install(FILES "${STAGED_JAR}" DESTINATION ${CMAKE_INSTALL_LIBDIR}/${MODULE_NAME})
    endif ()
endfunction ()

function (add_cola_gradle_library MODULE_NAME)
    cmake_parse_arguments(
        ARG "NO_INSTALL" "PROJECT_DIR;TASK;JAR"
        "GENERATORS;CONVERTERS;UNSAFE_GENERATORS;UNSAFE_CONVERTERS;WRITERS;GRADLE_ARGUMENTS"
        ${ARGN}
    )

    if (NOT MODULE_NAME)
        message(FATAL_ERROR "add_cola_gradle_library requires a module name")
    endif ()
    if (NOT ARG_JAR)
        message(FATAL_ERROR "add_cola_gradle_library requires JAR")
    endif ()
    if (ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR "Unknown add_cola_gradle_library arguments: ${ARG_UNPARSED_ARGUMENTS}")
    endif ()
    if (NOT ARG_PROJECT_DIR)
        set(ARG_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    endif ()
    if (NOT ARG_TASK)
        set(ARG_TASK colaJar)
    endif ()

    find_program(GRADLE_EXECUTABLE gradle REQUIRED)
    get_filename_component(
        GRADLE_PROJECT_DIR "${ARG_PROJECT_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}"
    )
    get_filename_component(JAR "${ARG_JAR}" ABSOLUTE BASE_DIR "${GRADLE_PROJECT_DIR}")

    add_custom_target(
        ${MODULE_NAME}-gradle
        COMMAND "${GRADLE_EXECUTABLE}" --no-daemon "-PcolaJvmApi=${COLA_JVM_API_JAR}"
                ${ARG_GRADLE_ARGUMENTS} "${ARG_TASK}"
        BYPRODUCTS "${JAR}"
        WORKING_DIRECTORY "${GRADLE_PROJECT_DIR}"
        VERBATIM
    )

    set(NO_INSTALL "")
    if (ARG_NO_INSTALL)
        set(NO_INSTALL NO_INSTALL)
    endif ()
    add_cola_jvm_library(
        ${MODULE_NAME}
        ${NO_INSTALL}
        JAR
        "${JAR}"
        DEPENDS
        ${MODULE_NAME}-gradle
        GENERATORS
        ${ARG_GENERATORS}
        CONVERTERS
        ${ARG_CONVERTERS}
        UNSAFE_GENERATORS
        ${ARG_UNSAFE_GENERATORS}
        UNSAFE_CONVERTERS
        ${ARG_UNSAFE_CONVERTERS}
        WRITERS
        ${ARG_WRITERS}
    )
endfunction ()
