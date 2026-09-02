plugins {
    java
    kotlin("jvm") version "2.2.0"
}

repositories { mavenCentral() }

dependencies {
    compileOnly("io.github.zakayz:cola-jvm-api:0.1.0")
}

java { toolchain { languageVersion.set(JavaLanguageVersion.of(17)) } }
kotlin { jvmToolchain(17) }
