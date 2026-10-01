cmake_minimum_required(VERSION 3.20)

project(
    COLA_MomentumScaler
    VERSION 0.1.0
    LANGUAGES CXX
)

find_package(COLA_JVM REQUIRED)

add_cola_gradle_library(
    MomentumScaler JAR "build/libs/cola-momentum-scaler-all.jar" CONVERTERS
    "momentum_scaler=org.cola.example.MomentumScaler"
)
