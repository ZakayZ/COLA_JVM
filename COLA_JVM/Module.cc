#include "COLA_JVM/JvmFilters.hh"  // NOLINT(misc-include-cleaner): supplies the exported module type.

extern "C" cola::VModule* LoadCOLAModule() { return new cola::jvm::JvmModule(); }
