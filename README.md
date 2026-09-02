# COLA JVM

`COLA_JVM` loads COLA generators, converters, and writers implemented in ordinary Java or Kotlin JARs. The native
host embeds one standard JVM per process through the JNI Invocation API and uses
[Google JNI Bind](https://github.com/google/jni-bind) for type-safe calls and JNI reference management.

## Requirements

- COLA
- JDK 17 or newer (a JRE alone does not contain the JNI headers and link library)
- SWIG 4 or newer, only when regenerating the checked-in wrappers
- CMake 3.20 or newer
- Maven 3.9 or newer, for building or publishing the Maven artifact
- GnuPG, only when publishing the Maven artifact
- A network connection for the first configure, unless `FETCHCONTENT_SOURCE_DIR_JNI_BIND` points to a local JNI Bind
  checkout

On macOS the JDK and Maven may be installed with `brew install openjdk maven`. Confirm that Maven is available with
`mvn --version`. JNI Bind itself has no Homebrew formula; it is header-only and CMake fetches the pinned
`Release-1.5.1` source.

## Build

```sh
export JAVA_HOME=$(/usr/libexec/java_home -v 21) # macOS example
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/cola/install
cmake --build build
cmake --install build
```

For an offline build, download JNI Bind once and configure with
`-DFETCHCONTENT_SOURCE_DIR_JNI_BIND=/path/to/jni-bind`.

The build produces `libCOLA_JVM` and `cola-jvm-api.jar`. `EventData`, `EventInitialState`, `EventParticles`, `Particle`,
and `LorentzVector` in the API JAR are generated from the COLA C++ headers by SWIG; they are native-backed proxies
rather than duplicated Java data classes.

The same API is packaged for Maven repositories as `io.github.zakayz:cola-jvm-api:0.1.0`. Build the main, sources,
and Javadoc JARs without building the native module with:

```sh
mvn clean verify
```

Use `mvn install` to put the artifacts in the local Maven repository for testing. CMake and Maven compile the same
checked-in Java and SWIG sources: CMake supplies the API JAR beside the installed native module at runtime, while
Maven produces the public compile-time dependency.

Generated Java and C++ sources live in `swig_gen/`, matching `COLA_Fortran`. A normal build uses those sources and
does not require SWIG. To regenerate them after changing `cola.i` or the wrapped COLA headers, run:

```sh
cmake -S . -B build -DBUILD_WITH_SWIG=ON
cmake --build build
```

The regenerated files are copied back into `swig_gen/` after the native library is built.

## Code quality

The project uses the same Google-derived C++ and CMake styles as `COLA_Fortran`:

```sh
cmake --build build --target clang-format-check
cmake --build build --target clang-tidy-check
cmake --build build --target cmake-format-check
```

Corresponding `*-fix` targets are available for ClangFormat, Clang-Tidy, and CMake Format. A target is created only
when its tool is available during CMake configuration.

## Implement a filter

User code depends on `cola-jvm-api.jar` and implements one of `org.cola.api.Generator`, `Converter`, or `Writer`.
Both a public no-argument constructor and a public `Map<String, String>` constructor are supported; the map
constructor is preferred when both exist.

Published releases can be used from Gradle:

```kotlin
repositories { mavenCentral() }

dependencies {
    compileOnly("io.github.zakayz:cola-jvm-api:0.1.0")
}
```

or Maven:

```xml
<dependency>
  <groupId>io.github.zakayz</groupId>
  <artifactId>cola-jvm-api</artifactId>
  <version>0.1.0</version>
  <scope>provided</scope>
</dependency>
```

The dependency must not be bundled into a filter JAR. The native COLA module supplies the matching API JAR to the
embedded JVM.

```kotlin
class MomentumScaler(metadata: Map<String, String>) : Converter {
    private val scale = metadata.getValue("scale").toDouble()

    override fun convert(event: EventData): EventData {
        event.particles.forEach { it.momentum.e *= scale }
        return event
    }
}
```

See `examples/momentum-scaler` for a Gradle Kotlin module.

## Use from COLA XML

Load the installed `COLA_JVM` module in the normal COLA driver, then select one of its three factories:

```xml
<root>
  <generator name="some_native_generator" />
  <converter name="jvm_converter"
             jar="/absolute/path/to/cola-momentum-scaler.jar"
             class="org.cola.example.MomentumScaler"
             scale="2.5" />
  <writer name="some_native_writer" />
</root>
```

Required JVM filter attributes are `jar` and `class`. All attributes, including custom ones such as `scale`, are
passed to the implementation constructor. `cola-jvm-api.jar` is installed beside the native library and discovered
automatically. `COLA_JVM_API_JAR` remains available as a development override.

The JVM is intentionally kept alive for the process lifetime and native threads are attached on demand. Event data is
not serialized: Java filters operate on native-backed proxies. As in `COLA_PY`, generator and converter results are
copied into the C++ pipeline, while converter inputs and writer inputs are transferred to Java. Java filters may
therefore retain any `EventData` they receive or return without creating dangling native proxies.

For explicitly zero-copy output, implement `UnsafeGenerator` or `UnsafeConverter` and select
`jvm_unsafe_generator` or `jvm_unsafe_converter` in XML. The returned event is forcefully transferred to C++ and its
Java proxy is invalidated immediately after the callback returns to the native caller. `EventData.isValid()` can be
used to inspect this state. An unsafe converter may return its input for an entirely zero-copy pass through the filter.

## Publish the Java API

Maven Central publication uses the `central` Maven profile and intentionally stops after validation so a release can
be reviewed before it becomes immutable.

1. Sign in to [Maven Central](https://central.sonatype.com/) with the `ZakayZ` GitHub account and verify the
   `io.github.zakayz` namespace.
2. Generate a Central Portal user token and expose it to Maven without committing it. For example, place this in
   `~/.m2/settings.xml`:

   ```xml
   <settings xmlns="http://maven.apache.org/SETTINGS/1.2.0"
             xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
             xsi:schemaLocation="http://maven.apache.org/SETTINGS/1.2.0
                                 https://maven.apache.org/xsd/settings-1.2.0.xsd">
     <servers>
       <server>
         <id>central</id>
         <username>${env.CENTRAL_USERNAME}</username>
         <password>${env.CENTRAL_PASSWORD}</password>
       </server>
     </servers>
   </settings>
   ```

3. Create or import the release GPG key. The Maven GPG plugin uses the local `gpg` keyring and agent by default.
4. Run `mvn clean verify` and inspect the main, sources, and Javadoc JARs in `target/`.
5. Keep the versions in `pom.xml` and `CMakeLists.txt` identical, commit and tag the release, then upload it:

   ```sh
   mvn -Pcentral clean deploy
   ```

6. Review the validated deployment in the Central Portal and select **Publish**.

The repository contains no publishing token, GPG private key, or passphrase. Published Maven coordinates cannot be
reused with changed contents; fixes require a new version.
