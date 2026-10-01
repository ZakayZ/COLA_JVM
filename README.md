# COLA JVM

Java and Kotlin bindings for [COLA](https://github.com/Spectator-matter-group-INR-RAS/COLA).
Write physics filters as JVM classes and use them in COLA calculations.

## Quick start

Requires an installed COLA, JDK 17+, CMake 3.20+ and a C++20 compiler.
The first configuration downloads JNI Bind.

```shell
export COLA_DIR="/path/to/cola/install"
cmake -S . -B build -DCMAKE_PREFIX_PATH="$COLA_DIR" -DCMAKE_INSTALL_PREFIX="$COLA_DIR"
cmake --build build
cmake --install build
```

Run using the [COLA-PY](https://github.com/ZakayZ/COLA-PY) CLI:

```shell
cola run --config config.xml --library COLA_JVM
```

Example entry in your calculation XML:

```xml
<converter name="jvm_converter"
           jar="/path/to/myphysics.jar"
           class="org.myphysics.ScaleConverter"
           scale="2"/>
```

## Write a filter

Depend on the installed `cola-jvm-api.jar` and implement `org.cola.api.Generator`,
`Converter` or `Writer`. A public `Map<String, String>` constructor receives XML
attributes; a public no-argument constructor is also supported.
Do not bundle the COLA API into your filter JAR.

See the [Java](examples/java-scaler) and [Kotlin](examples/momentum-scaler) examples.
Their Gradle/CMake setup installs a JAR alongside a generated native module,
so calculations can select filters by name without specifying JAR paths.
Building these examples also requires Gradle.

## Tests

```shell
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Includes direct Java bridge tests and C++ integration tests.
