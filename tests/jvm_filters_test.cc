#include "COLA_JVM/JvmFilters.hh"

#include <cmath>
#include <exception>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {

  std::unordered_map<std::string, std::string> FilterMetadata(const std::string& jar, const std::string& class_name) {
    return {{"jar", jar}, {"class", class_name}};
  }

  void Require(bool condition, const char* message) {
    if (!condition) {
      throw std::runtime_error(message);
    }
  }

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: cola-jvm-filters-test TEST_MODULE.jar\n";
    return 2;
  }

  try {
    const auto jar = std::string{argv[1]};
    auto generator = cola::jvm::JvmGenerator(FilterMetadata(jar, "org.cola.test.TestGenerator"));
    auto event = generator();
    Require(event != nullptr, "generator returned null");
    Require(event->ini_state.energy == 42.0, "copying the generator result lost event state");
    Require(event->particles.size() == 1, "copying the generator result lost particles");
    event = generator();

    event = cola::jvm::JvmConverter(FilterMetadata(jar, "org.cola.example.ScaleConverter"))(std::move(event));
    Require(std::abs(event->particles.front().momentum.e - 10.0) < 1.0e-12,
            "converter output copy lost the JVM mutation");

    auto converter = cola::jvm::JvmConverter(FilterMetadata(jar, "org.cola.test.ReplacementConverter"));
    event = converter(std::move(event));
    Require(event->ini_state.energy == 77.0, "copying the replacement converter result lost event state");
    event = converter(generator());

    auto writer = cola::jvm::JvmWriter(FilterMetadata(jar, "org.cola.test.TestWriter"));
    writer(generator());
    writer(std::move(event));

    auto unsafe_generator = cola::jvm::JvmUnsafeGenerator(FilterMetadata(jar, "org.cola.test.TestUnsafeGenerator"));
    event = unsafe_generator();
    Require(event->ini_state.energy == 13.0, "unsafe generator ownership transfer lost event state");
    event = unsafe_generator();

    auto unsafe_converter = cola::jvm::JvmUnsafeConverter(FilterMetadata(jar, "org.cola.test.TestUnsafeConverter"));
    event = unsafe_converter(std::move(event));
    Require(event->ini_state.energy == 17.0, "unsafe converter ownership transfer lost event state");
    event = unsafe_converter(std::move(event));

    const auto factories = cola::jvm::JvmModule{}.GetModuleFilters();
    Require(factories.contains("jvm_unsafe_generator"), "module does not expose the unsafe JVM generator");
    Require(factories.contains("jvm_unsafe_converter"), "module does not expose the unsafe JVM converter");
    auto generic_filter = factories.at("jvm_generator")->Create(FilterMetadata(jar, "org.cola.test.TestGenerator"));
    auto* generic_generator = dynamic_cast<cola::VGenerator*>(generic_filter.get());
    Require(generic_generator != nullptr, "generic factory returned the wrong JVM filter type");
    event = (*generic_generator)();
    Require(event->ini_state.energy == 42.0, "generic factory did not pass metadata to the JVM filter");
  } catch (const std::exception& error) {
    std::cerr << "cola-jvm-filters-test: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
