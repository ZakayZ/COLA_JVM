#include "COLA_JVM/JvmFilters.hh"

#include "COLA_JVM/JniRuntime.hh"

#include <jni_bind_release.h>

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace cola::jvm {
  namespace {

    using detail::ThrowIfJavaException;

    constexpr auto kEventData = jni::Class{
        "org/cola/api/EventData", jni::Static{jni::Method{"ownNative", jni::Return{jni::Self{}}, jni::Params{jlong{}}}},
        jni::Method{"releaseToNative", jni::Return{jlong{}}}, jni::Method{"nativeAddress", jni::Return{jlong{}}}};

    // Same order as Bridge.Kind.
    enum class FilterKind : jint { kGenerator, kConverter, kUnsafeGenerator, kUnsafeConverter, kWriter };

    constexpr auto kFilterHandle =
        jni::Class{"org/cola/api/internal/Bridge$FilterHandle",
                   jni::Method{"process", jni::Return{kEventData}, jni::Params{kEventData}},
                   jni::Method{"lastCallbackNanoseconds", jni::Return{jlong{}}}, jni::Method{"close", jni::Return{}}};
    constexpr auto kFilterLoader =
        jni::Class{"org/cola/api/internal/Bridge$FilterLoader", jni::Constructor{},
                   jni::Method{"putMetadata", jni::Return{}, jni::Params{jstring{}, jstring{}}},
                   jni::Method{"load", jni::Return{kFilterHandle}, jni::Params{jint{}}}};
    constexpr auto kBridge =
        jni::Class{"org/cola/api/internal/Bridge",
                   jni::Static{jni::Method{"loadNativeLibrary", jni::Return{}, jni::Params{jstring{}}}}};

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

    jni::GlobalObject<kFilterHandle> LoadFilter(detail::JvmRuntime& runtime,
                                                const std::unordered_map<std::string, std::string>& metadata,
                                                FilterKind kind) {
      auto& env = runtime.Env();
      jni::StaticRef<kBridge>{}.Call<"loadNativeLibrary">(detail::NativeLibraryPath().string());
      ThrowIfJavaException(env, "registering the COLA JVM native proxy library");

      auto loader = jni::LocalObject<kFilterLoader>{};
      RequireJavaObject(env, loader, "creating JVM filter loader");
      for (const auto& [key, value] : metadata) {
        loader.Call<"putMetadata">(key, value);
        ThrowIfJavaException(env, "inserting JVM filter metadata");
      }

      auto filter = loader.Call<"load">(static_cast<jint>(kind));
      RequireJavaObject(env, filter, "loading JVM filter");
      auto object = jni::GlobalObject<kFilterHandle>{jni::PromoteToGlobal{}, filter.Release()};
      RequireJavaObject(env, object, "retaining JVM filter handle");
      return object;
    }

    template <FilterKind Kind>
    struct FilterImpl {
      explicit FilterImpl(const std::unordered_map<std::string, std::string>& metadata)
          : runtime(detail::JvmRuntime::Instance()), object(LoadFilter(runtime, metadata, Kind)) {}

      ~FilterImpl() {
        try {
          auto& env = runtime.Env();
          object.Call<"close">();
          ThrowIfJavaException(env, "closing JVM filter");
        } catch (const std::exception& error) {
          std::cerr << error.what() << '\n';
        }
      }

      detail::JvmRuntime& runtime;
      jni::GlobalObject<kFilterHandle> object;

      std::uint64_t LastCallbackNanoseconds() {
        auto& env = runtime.Env();
        auto value = object.Call<"lastCallbackNanoseconds">();
        ThrowIfJavaException(env, "reading JVM converter callback time");
        return static_cast<std::uint64_t>(value);
      }
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

  struct JvmGenerator::Impl final : FilterImpl<FilterKind::kGenerator> {
    using FilterImpl::FilterImpl;
  };

  struct JvmConverter::Impl final : FilterImpl<FilterKind::kConverter> {
    using FilterImpl::FilterImpl;
  };

  struct JvmUnsafeGenerator::Impl final : FilterImpl<FilterKind::kUnsafeGenerator> {
    using FilterImpl::FilterImpl;
  };

  struct JvmUnsafeConverter::Impl final : FilterImpl<FilterKind::kUnsafeConverter> {
    using FilterImpl::FilterImpl;
  };

  struct JvmWriter::Impl final : FilterImpl<FilterKind::kWriter> {
    using FilterImpl::FilterImpl;
  };

  JvmGenerator::JvmGenerator(const std::unordered_map<std::string, std::string>& metadata)
      : impl_(std::make_unique<Impl>(metadata)) {}

  JvmGenerator::~JvmGenerator() = default;

  std::unique_ptr<EventData> JvmGenerator::operator()() {
    auto& env = impl_->runtime.Env();

    auto result = impl_->object.Call<"process">(jni::LocalObject<kEventData>{nullptr});
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

    auto result = impl_->object.Call<"process">(MoveEventToJava(env, std::move(data)));
    RequireJavaObject(env, result, "running JVM converter");

    const auto address = result.Call<"nativeAddress">();
    RequireJavaObject(env, address, "copying JVM converter output to COLA");

    return CopyEvent(address);
  }

  std::uint64_t JvmConverter::LastCallbackNanoseconds() const { return impl_->LastCallbackNanoseconds(); }

  JvmUnsafeGenerator::JvmUnsafeGenerator(const std::unordered_map<std::string, std::string>& metadata)
      : impl_(std::make_unique<Impl>(metadata)) {}

  JvmUnsafeGenerator::~JvmUnsafeGenerator() = default;

  std::unique_ptr<EventData> JvmUnsafeGenerator::operator()() {
    auto& env = impl_->runtime.Env();

    auto result = impl_->object.Call<"process">(jni::LocalObject<kEventData>{nullptr});
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

    auto result = impl_->object.Call<"process">(MoveEventToJava(env, std::move(data)));
    RequireJavaObject(env, result, "running unsafe JVM converter");

    const auto address = result.Call<"releaseToNative">();
    RequireJavaObject(env, address, "taking unsafe JVM converter output into COLA");

    return TakeEvent(address);
  }

  std::uint64_t JvmUnsafeConverter::LastCallbackNanoseconds() const { return impl_->LastCallbackNanoseconds(); }

  JvmWriter::JvmWriter(const std::unordered_map<std::string, std::string>& metadata)
      : impl_(std::make_unique<Impl>(metadata)) {}

  JvmWriter::~JvmWriter() = default;

  void JvmWriter::operator()(std::unique_ptr<EventData>&& data) {
    auto& env = impl_->runtime.Env();
    impl_->object.Call<"process">(MoveEventToJava(env, std::move(data)));
    ThrowIfJavaException(env, "running JVM writer");
  }

}  // namespace cola::jvm
