cmake_minimum_required(VERSION 3.20)

project(
    COLA_JavaScaler
    VERSION 0.1.0
    LANGUAGES CXX
)

find_package(COLA_JVM REQUIRED)

add_cola_gradle_library(
    JavaScaler JAR "build/libs/cola-java-scaler-all.jar" CONVERTERS
    "scale_converter=org.cola.example.ScaleConverter"
)
