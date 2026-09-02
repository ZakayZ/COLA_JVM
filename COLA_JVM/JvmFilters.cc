#include "COLA_JVM/JvmFilters.hh"

#include "COLA_JVM/JniRuntime.hh"

#include <jni_bind_release.h>

#include <cstdint>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>

namespace cola::jvm {
  namespace {

    using detail::ThrowIfJavaException;

    struct GeneratorFilter final {};
    struct ConverterFilter final {};
    struct UnsafeGeneratorFilter final {};
    struct UnsafeConverterFilter final {};
    struct WriterFilter final {};

    constexpr auto kEventData = jni::Class{
        "org/cola/api/EventData", jni::Static{jni::Method{"ownNative", jni::Return{jni::Self{}}, jni::Params{jlong{}}}},
        jni::Method{"releaseToNative", jni::Return{jlong{}}}, jni::Method{"nativeAddress", jni::Return{jlong{}}}};

    template <typename Filter>
    struct FilterDefinition;

    template <>
    struct FilterDefinition<GeneratorFilter> final {
      static constexpr auto kCls =
          jni::Class{"org/cola/api/Generator", jni::Method{"generate", jni::Return{kEventData}}};
      static constexpr auto kLoaderMethod = jni::metaprogramming::StringLiteral{"loadGenerator"};
      static constexpr auto kLoader = jni::Method{kLoaderMethod.value, jni::Return{kCls}};
    };

    template <>
    struct FilterDefinition<ConverterFilter> final {
      static constexpr auto kCls = jni::Class{"org/cola/api/Converter",
                                              jni::Method{"convert", jni::Return{kEventData}, jni::Params{kEventData}}};
      static constexpr auto kLoaderMethod = jni::metaprogramming::StringLiteral{"loadConverter"};
      static constexpr auto kLoader = jni::Method{kLoaderMethod.value, jni::Return{kCls}};
    };

    template <>
    struct FilterDefinition<UnsafeGeneratorFilter> final {
      static constexpr auto kCls =
          jni::Class{"org/cola/api/UnsafeGenerator", jni::Method{"generate", jni::Return{kEventData}}};
      static constexpr auto kLoaderMethod = jni::metaprogramming::StringLiteral{"loadUnsafeGenerator"};
      static constexpr auto kLoader = jni::Method{kLoaderMethod.value, jni::Return{kCls}};
    };

    template <>
    struct FilterDefinition<UnsafeConverterFilter> final {
      static constexpr auto kCls = jni::Class{"org/cola/api/UnsafeConverter",
                                              jni::Method{"convert", jni::Return{kEventData}, jni::Params{kEventData}}};
      static constexpr auto kLoaderMethod = jni::metaprogramming::StringLiteral{"loadUnsafeConverter"};
      static constexpr auto kLoader = jni::Method{kLoaderMethod.value, jni::Return{kCls}};
    };

    template <>
    struct FilterDefinition<WriterFilter> final {
      static constexpr auto kCls =
          jni::Class{"org/cola/api/Writer", jni::Method{"write", jni::Return{}, jni::Params{kEventData}}};
      static constexpr auto kLoaderMethod = jni::metaprogramming::StringLiteral{"loadWriter"};
      static constexpr auto kLoader = jni::Method{kLoaderMethod.value, jni::Return{kCls}};
    };

    constexpr auto kFilterLoader =
        jni::Class{"org/cola/api/internal/Bridge$FilterLoader",
                   jni::Constructor{jstring{}, jstring{}},
                   jni::Method{"putMetadata", jni::Return{}, jni::Params{jstring{}, jstring{}}},
                   FilterDefinition<GeneratorFilter>::kLoader,
                   FilterDefinition<ConverterFilter>::kLoader,
                   FilterDefinition<UnsafeGeneratorFilter>::kLoader,
                   FilterDefinition<UnsafeConverterFilter>::kLoader,
                   FilterDefinition<WriterFilter>::kLoader};
    constexpr auto kBridge =
        jni::Class{"org/cola/api/internal/Bridge",
                   jni::Static{jni::Method{"loadNativeLibrary", jni::Return{}, jni::Params{jstring{}}}}};

    std::string RequireMetadata(const std::unordered_map<std::string, std::string>& metadata, const std::string& key) {
      if (const auto value = metadata.find(key); value != metadata.end() && !value->second.empty()) {
        return value->second;
      }
      throw std::invalid_argument(std::string("JVM filter requires a non-empty '") + key + "' XML attribute");
    }

    void RequireJavaObject(JNIEnv& env, const jni::RefBase<jobject>& object, const std::string& context) {
      ThrowIfJavaException(env, context);
      if (static_cast<jobject>(object) == nullptr) {
        throw std::runtime_error("JVM returned null while: " + context);
      }
    }

    void RequireJavaObject(JNIEnv& env, jlong address, const std::string& context) {
      ThrowIfJavaException(env, context);
      if (address == 0) {
        throw std::runtime_error("JVM returned null while: " + context);
      }
    }

    void EnsureNativeLibraryLoaded(JNIEnv& env) {
      static std::once_flag loaded;
      std::call_once(loaded, [&env] {
        jni::StaticRef<kBridge>{}.Call<"loadNativeLibrary">(detail::NativeLibraryPath().string());
        ThrowIfJavaException(env, "registering the COLA JVM native proxy library");
      });
    }

    template <typename Filter>
    jni::GlobalObject<FilterDefinition<Filter>::kCls> LoadFilter(
        detail::JvmRuntime& runtime, const std::unordered_map<std::string, std::string>& metadata) {
      auto& env = runtime.Env();
      EnsureNativeLibraryLoaded(env);

      const auto class_name = RequireMetadata(metadata, "class");
      auto loader = jni::LocalObject<kFilterLoader>{RequireMetadata(metadata, "jar"), class_name};
      ThrowIfJavaException(env, "creating JVM filter loader");

      for (const auto& [key, value] : metadata) {
        loader.Call<"putMetadata">(key, value);
        ThrowIfJavaException(env, "inserting JVM filter metadata");
      }

      auto filter = loader.template Call<FilterDefinition<Filter>::kLoaderMethod>();
      RequireJavaObject(env, filter, "loading JVM filter " + class_name);

      auto object = jni::GlobalObject<FilterDefinition<Filter>::kCls>{jni::PromoteToGlobal{}, filter.Release()};
      RequireJavaObject(env, object, "retaining JVM filter instance");

      return object;
    }

    template <typename Filter>
    struct FilterImpl {
      explicit FilterImpl(const std::unordered_map<std::string, std::string>& metadata)
          : runtime(detail::JvmRuntime::Instance()), object(LoadFilter<Filter>(runtime, metadata)) {}

      ~FilterImpl() { static_cast<void>(runtime.Env()); }

      detail::JvmRuntime& runtime;
      jni::GlobalObject<FilterDefinition<Filter>::kCls> object;
    };

    jlong EventAddress(const EventData* event) {
      static_assert(sizeof(std::uintptr_t) <= sizeof(jlong));
      return static_cast<jlong>(reinterpret_cast<std::uintptr_t>(event));
    }

    EventData* EventAt(jlong address) {
      // NOLINTNEXTLINE(performance-no-int-to-ptr)
      return reinterpret_cast<EventData*>(static_cast<std::uintptr_t>(address));
    }

    std::unique_ptr<EventData> CopyEvent(jlong address) { return std::make_unique<EventData>(*EventAt(address)); }

    std::unique_ptr<EventData> TakeEvent(jlong address) { return std::unique_ptr<EventData>{EventAt(address)}; }

    jni::LocalObject<kEventData> MoveEventToJava(JNIEnv& env, std::unique_ptr<EventData>&& event) {
      auto proxy = jni::StaticRef<kEventData>{}.Call<"ownNative">(EventAddress(event.get()));
      RequireJavaObject(env, proxy, "transferring EventData ownership to the JVM");

      event.release();  // NOLINT(bugprone-unused-return-value) ownership moved to proxy
      return proxy;
    }

  }  // namespace

  struct JvmGenerator::Impl final : FilterImpl<GeneratorFilter> {
    using FilterImpl::FilterImpl;
  };

  struct JvmConverter::Impl final : FilterImpl<ConverterFilter> {
    using FilterImpl::FilterImpl;
  };

  struct JvmUnsafeGenerator::Impl final : FilterImpl<UnsafeGeneratorFilter> {
    using FilterImpl::FilterImpl;
  };

  struct JvmUnsafeConverter::Impl final : FilterImpl<UnsafeConverterFilter> {
    using FilterImpl::FilterImpl;
  };

  struct JvmWriter::Impl final : FilterImpl<WriterFilter> {
    using FilterImpl::FilterImpl;
  };

  JvmGenerator::JvmGenerator(const std::unordered_map<std::string, std::string>& metadata)
      : impl_(std::make_unique<Impl>(metadata)) {}

  JvmGenerator::~JvmGenerator() = default;

  std::unique_ptr<EventData> JvmGenerator::operator()() {
    auto& env = impl_->runtime.Env();

    auto result = impl_->object.Call<"generate">();
    RequireJavaObject(env, result, "running JVM generator");

    const auto address = result.Call<"nativeAddress">();
    RequireJavaObject(env, address, "copying JVM generator output to COLA");

    return CopyEvent(address);
  }

  JvmConverter::JvmConverter(const std::unordered_map<std::string, std::string>& metadata)
      : impl_(std::make_unique<Impl>(metadata)) {}

  JvmConverter::~JvmConverter() = default;

  std::unique_ptr<EventData> JvmConverter::operator()(std::unique_ptr<EventData>&& data) {
    auto& env = impl_->runtime.Env();

    auto result = impl_->object.Call<"convert">(MoveEventToJava(env, std::move(data)));
    RequireJavaObject(env, result, "running JVM converter");

    const auto address = result.Call<"nativeAddress">();
    RequireJavaObject(env, address, "copying JVM converter output to COLA");

    return CopyEvent(address);
  }

  JvmUnsafeGenerator::JvmUnsafeGenerator(const std::unordered_map<std::string, std::string>& metadata)
      : impl_(std::make_unique<Impl>(metadata)) {}

  JvmUnsafeGenerator::~JvmUnsafeGenerator() = default;

  std::unique_ptr<EventData> JvmUnsafeGenerator::operator()() {
    auto& env = impl_->runtime.Env();

    auto result = impl_->object.Call<"generate">();
    RequireJavaObject(env, result, "running unsafe JVM generator");

    const auto address = result.Call<"releaseToNative">();
    RequireJavaObject(env, address, "taking unsafe JVM generator output into COLA");

    return TakeEvent(address);
  }

  JvmUnsafeConverter::JvmUnsafeConverter(const std::unordered_map<std::string, std::string>& metadata)
      : impl_(std::make_unique<Impl>(metadata)) {}

  JvmUnsafeConverter::~JvmUnsafeConverter() = default;

  std::unique_ptr<EventData> JvmUnsafeConverter::operator()(std::unique_ptr<EventData>&& data) {
    auto& env = impl_->runtime.Env();

    auto result = impl_->object.Call<"convert">(MoveEventToJava(env, std::move(data)));
    RequireJavaObject(env, result, "running unsafe JVM converter");

    const auto address = result.Call<"releaseToNative">();
    RequireJavaObject(env, address, "taking unsafe JVM converter output into COLA");

    return TakeEvent(address);
  }

  JvmWriter::JvmWriter(const std::unordered_map<std::string, std::string>& metadata)
      : impl_(std::make_unique<Impl>(metadata)) {}

  JvmWriter::~JvmWriter() = default;

  void JvmWriter::operator()(std::unique_ptr<EventData>&& data) {
    auto& env = impl_->runtime.Env();
    impl_->object.Call<"write">(MoveEventToJava(env, std::move(data)));
    ThrowIfJavaException(env, "running JVM writer");
  }

}  // namespace cola::jvm
