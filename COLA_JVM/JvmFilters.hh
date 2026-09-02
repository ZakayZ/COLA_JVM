#ifndef COLA_JVM_JVM_FILTERS_HH
#define COLA_JVM_JVM_FILTERS_HH

#include <COLA.hh>

#include <memory>
#include <string>
#include <unordered_map>

namespace cola::jvm {

  class JvmGenerator final : public VGenerator {
   public:
    explicit JvmGenerator(const std::unordered_map<std::string, std::string>& metadata);
    ~JvmGenerator() override;
    std::unique_ptr<EventData> operator()() override;
    inline static const std::string kName{"jvm_generator"};

   private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };

  class JvmConverter final : public VConverter {
   public:
    explicit JvmConverter(const std::unordered_map<std::string, std::string>& metadata);
    ~JvmConverter() override;
    std::unique_ptr<EventData> operator()(std::unique_ptr<EventData>&& data) override;
    inline static const std::string kName{"jvm_converter"};

   private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };

  class JvmUnsafeGenerator final : public VGenerator {
   public:
    explicit JvmUnsafeGenerator(const std::unordered_map<std::string, std::string>& metadata);
    ~JvmUnsafeGenerator() override;
    std::unique_ptr<EventData> operator()() override;
    inline static const std::string kName{"jvm_unsafe_generator"};

   private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };

  class JvmUnsafeConverter final : public VConverter {
   public:
    explicit JvmUnsafeConverter(const std::unordered_map<std::string, std::string>& metadata);
    ~JvmUnsafeConverter() override;
    std::unique_ptr<EventData> operator()(std::unique_ptr<EventData>&& data) override;
    inline static const std::string kName{"jvm_unsafe_converter"};

   private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };

  class JvmWriter final : public VWriter {
   public:
    explicit JvmWriter(const std::unordered_map<std::string, std::string>& metadata);
    ~JvmWriter() override;
    void operator()(std::unique_ptr<EventData>&& data) override;
    inline static const std::string kName{"jvm_writer"};

   private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
  };

  using JvmGeneratorFactory = GenericFactory<JvmGenerator>;
  using JvmConverterFactory = GenericFactory<JvmConverter>;

  using JvmUnsafeGeneratorFactory = GenericFactory<JvmUnsafeGenerator>;
  using JvmUnsafeConverterFactory = GenericFactory<JvmUnsafeConverter>;

  using JvmWriterFactory = GenericFactory<JvmWriter>;

  using JvmModule = GenericModule<JvmGeneratorFactory, JvmConverterFactory, JvmUnsafeGeneratorFactory,
                                  JvmUnsafeConverterFactory, JvmWriterFactory>;

}  // namespace cola::jvm

#endif
