#ifndef COLA_JVM_JNI_RUNTIME_HH
#define COLA_JVM_JNI_RUNTIME_HH

#include <jni.h>
#include <jni_bind_release.h>

#include <filesystem>
#include <string>

namespace cola::jvm::detail {

  void ThrowIfJavaException(JNIEnv& env, const std::string& context);
  const std::filesystem::path& NativeLibraryPath();

  class JvmRuntime final {
   public:
    static JvmRuntime& Instance();

    JvmRuntime(const JvmRuntime&) = delete;
    JvmRuntime& operator=(const JvmRuntime&) = delete;
    ~JvmRuntime();

    JNIEnv& Env() const;

   private:
    JvmRuntime();
    JNIEnv* AttachedEnv() const noexcept;

    jni::JvmRef<jni::kDefaultJvm> reference_;
  };

}  // namespace cola::jvm::detail

#endif
