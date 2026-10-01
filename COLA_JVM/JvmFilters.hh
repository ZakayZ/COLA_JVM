#ifndef COLA_JVM_JVM_FILTERS_HH
#define COLA_JVM_JVM_FILTERS_HH

#include <COLA.hh>

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>

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

  template <typename Filter, typename FactoryBase>
  class GeneratedJvmFactory final : public FactoryBase {
   public:
    GeneratedJvmFactory(std::string name, std::string jar, std::string class_name)
        : name_(std::move(name)), jar_(std::move(jar)), class_name_(std::move(class_name)) {}

    std::unique_ptr<VFilter> Create(const std::unordered_map<std::string, std::string>& metadata) override {
      std::unordered_map<std::string, std::string> generated_metadata{metadata};
      generated_metadata.insert_or_assign("jar", jar_);
      generated_metadata.insert_or_assign("class", class_name_);
      return std::make_unique<Filter>(generated_metadata);
    }

    const std::string& GetFilterName() const override { return name_; }

   private:
    std::string name_;
    std::string jar_;
    std::string class_name_;
  };

  using JvmGeneratorFactory = GenericFactory<JvmGenerator>;
  using JvmConverterFactory = GenericFactory<JvmConverter>;

  using JvmUnsafeGeneratorFactory = GenericFactory<JvmUnsafeGenerator>;
  using JvmUnsafeConverterFactory = GenericFactory<JvmUnsafeConverter>;

  using JvmWriterFactory = GenericFactory<JvmWriter>;

  using GeneratedJvmGeneratorFactory = GeneratedJvmFactory<JvmGenerator, VGeneratorFactory>;
  using GeneratedJvmConverterFactory = GeneratedJvmFactory<JvmConverter, VConverterFactory>;

  using GeneratedJvmUnsafeGeneratorFactory = GeneratedJvmFactory<JvmUnsafeGenerator, VGeneratorFactory>;
  using GeneratedJvmUnsafeConverterFactory = GeneratedJvmFactory<JvmUnsafeConverter, VConverterFactory>;

  using GeneratedJvmWriterFactory = GeneratedJvmFactory<JvmWriter, VWriterFactory>;

  using JvmModule = GenericModule<JvmGeneratorFactory, JvmConverterFactory, JvmUnsafeGeneratorFactory,
                                  JvmUnsafeConverterFactory, JvmWriterFactory>;

}  // namespace cola::jvm

#endif
