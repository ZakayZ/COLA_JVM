plugins { java }

repositories { mavenCentral() }

val colaJvmApi = providers.gradleProperty("colaJvmApi")

dependencies {
    if (colaJvmApi.isPresent) {
        compileOnly(files(colaJvmApi.get()))
    } else {
        compileOnly("io.github.zakayz:cola-jvm-api:0.1.0")
    }
}

tasks.register<Jar>("colaJar") {
    archiveFileName.set("cola-java-scaler-all.jar")
    duplicatesStrategy = DuplicatesStrategy.EXCLUDE
    from(sourceSets.main.get().output)
    dependsOn(configurations.runtimeClasspath)
    from({
        configurations.runtimeClasspath.get().map {
            if (it.isDirectory) it else zipTree(it)
        }
    })
}

java { toolchain { languageVersion.set(JavaLanguageVersion.of(17)) } }
