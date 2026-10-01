#include "COLA_JVM/JniRuntime.hh"

#include <dlfcn.h>

#include <cstdlib>
#include <stdexcept>

namespace cola::jvm::detail {
  namespace {

    constexpr auto kThrowable = jni::Class{"java/lang/Throwable"};
    constexpr auto kBridge =
        jni::Class{"org/cola/api/internal/Bridge",
                   jni::Static{jni::Method{"formatException", jni::Return{jstring{}}, jni::Params{kThrowable}}}};

    std::filesystem::path ApiJarPath() {
      if (const auto* value = std::getenv("COLA_JVM_API_JAR"); value != nullptr && *value != '\0') {
        return value;
      }
      return NativeLibraryPath().parent_path() / "cola-jvm-api.jar";
    }

    JavaVM* CreateJvm() {
      auto api_jar = ApiJarPath();
      if (!std::filesystem::exists(api_jar)) {
        throw std::runtime_error("COLA JVM API jar does not exist: " + api_jar.string());
      }
      api_jar = std::filesystem::canonical(api_jar);

      auto class_path = "-Djava.class.path=" + api_jar.string();
      auto option = JavaVMOption{.optionString = class_path.data(), .extraInfo = nullptr};
      auto arguments = JavaVMInitArgs{
          .version = JNI_VERSION_1_8, .nOptions = 1, .options = &option, .ignoreUnrecognized = JNI_FALSE};
      auto* vm = static_cast<JavaVM*>(nullptr);
      auto* env = static_cast<JNIEnv*>(nullptr);
      if (JNI_CreateJavaVM(&vm, reinterpret_cast<void**>(&env), &arguments) != JNI_OK) {
        throw std::runtime_error("COLA_JVM failed to create the embedded JVM");
      }
      return vm;
    }

    std::string JavaExceptionMessage(JNIEnv& env) {
      if (auto* exception = env.ExceptionOccurred(); exception != nullptr) {
        env.ExceptionClear();

        auto throwable = jni::LocalObject<kThrowable>{jni::AdoptLocal{}, static_cast<jobject>(exception)};
        auto text = jni::StaticRef<kBridge>{}.Call<"formatException">(throwable);
        if (env.ExceptionCheck() == JNI_TRUE) {
          env.ExceptionClear();
          return "Java exception (formatting failed)";
        }
        if (static_cast<jstring>(text) == nullptr) {
          return "Java exception";
        }

        if (auto message = text.PinAsStr().ToString(); env.ExceptionCheck() != JNI_TRUE) {
          return message;
        }
        env.ExceptionClear();
        return "Java exception (message could not be decoded)";
      }
      return "unknown Java exception";
    }

  }  // namespace

  void ThrowIfJavaException(JNIEnv& env, const std::string& context) {
    if (env.ExceptionCheck() == JNI_TRUE) {
      throw std::runtime_error(context + ": " + JavaExceptionMessage(env));
    }
  }

  const std::filesystem::path& NativeLibraryPath() {
    static const auto path = [] {
      auto information = Dl_info{};
      if (dladdr(reinterpret_cast<const void*>(&NativeLibraryPath), &information) == 0 ||
          information.dli_fname == nullptr) {
        throw std::runtime_error("COLA_JVM could not resolve its native library path");
      }
      return std::filesystem::canonical(information.dli_fname);
    }();
    return path;
  }

  JvmRuntime& JvmRuntime::Instance() {
    static auto runtime = JvmRuntime{};
    return runtime;
  }

  JvmRuntime::JvmRuntime() : reference_(CreateJvm()) {}

  JvmRuntime::~JvmRuntime() = default;

  JNIEnv* JvmRuntime::AttachedEnv() const noexcept {
    thread_local auto attachment = reference_.BuildThreadGuard();
    return jni::JniEnv::GetEnv();
  }

  JNIEnv& JvmRuntime::Env() const {
    if (auto* env = AttachedEnv(); env != nullptr) {
      return *env;
    }
    throw std::runtime_error("COLA_JVM could not attach the current native thread");
  }

}  // namespace cola::jvm::detail
