#include <COLA.hh>

#include <exception>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: cola-jvm-generated-module-test MODULE_DIRECTORY\n";
    return 2;
  }

  try {
    auto factories = cola::LoadModule("COLA_JVM_TestModule", argv[1])->GetModuleFilters();
    auto filter = factories.at("test_generator")->Create({});
    auto* generator = dynamic_cast<cola::VGenerator*>(filter.get());
    if (generator == nullptr) {
      throw std::runtime_error("generated factory returned the wrong filter type");
    }
    if (auto event = (*generator)(); event->ini_state.energy != 42.0) {
      throw std::runtime_error("generated module did not run its Java generator");
    }
  } catch (const std::exception& error) {
    std::cerr << "cola-jvm-generated-module-test: " << error.what() << '\n';
    return 1;
  }
  return 0;
}
