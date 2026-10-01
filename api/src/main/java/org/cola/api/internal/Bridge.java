package org.cola.api.internal;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.lang.reflect.InvocationTargetException;
import java.net.URL;
import java.net.URLClassLoader;
import java.nio.file.Path;
import java.util.HashMap;
import java.util.Map;
import java.util.Objects;
import org.cola.api.Converter;
import org.cola.api.EventData;
import org.cola.api.Generator;
import org.cola.api.UnsafeConverter;
import org.cola.api.UnsafeGenerator;
import org.cola.api.Writer;

public final class Bridge {
    private static boolean nativeLibraryLoaded;

    private Bridge() {}

    public static synchronized void loadNativeLibrary(String path) {
        if (!nativeLibraryLoaded) {
            System.load(path);
            nativeLibraryLoaded = true;
        }
    }

    public static String formatException(Throwable exception) {
        var text = new StringWriter();
        exception.printStackTrace(new PrintWriter(text));
        return text.toString();
    }

    /** Internal protocol shared with the native adapter; not user filter registration. */
    private enum Kind {
        GENERATOR(Generator.class),
        CONVERTER(Converter.class),
        UNSAFE_GENERATOR(UnsafeGenerator.class),
        UNSAFE_CONVERTER(UnsafeConverter.class),
        WRITER(Writer.class);

        private final Class<?> type;

        Kind(Class<?> type) {
            this.type = type;
        }
    }

    public static final class FilterLoader {
        private final Map<String, String> metadata = new HashMap<>();

        public FilterLoader() {}

        public void putMetadata(String key, String value) {
            metadata.put(key, value);
        }

        private String required(String key) {
            String value = metadata.get(key);
            if (value == null || value.isEmpty()) {
                throw new IllegalArgumentException(
                        "JVM filter requires a non-empty '" + key + "' XML attribute");
            }
            return value;
        }

        public FilterHandle load(int kindIndex) throws Throwable {
            Kind kind = Kind.values()[kindIndex];
            String className = required("class");
            var loader = new URLClassLoader(
                    new URL[] {Path.of(required("jar")).toAbsolutePath().toUri().toURL()},
                    Bridge.class.getClassLoader());
            try {
                Class<?> implementation = Class.forName(className, true, loader);
                implementation.asSubclass(kind.type);
                Object filter;
                try {
                    filter = implementation.getConstructor(Map.class).newInstance(metadata);
                } catch (NoSuchMethodException ignored) {
                    filter = implementation.getConstructor().newInstance();
                }
                return new FilterHandle(kind, filter, loader);
            } catch (Throwable failure) {
                Throwable cause = failure instanceof InvocationTargetException
                        ? ((InvocationTargetException) failure).getTargetException() : failure;
                try {
                    loader.close();
                } catch (Throwable closing) {
                    cause.addSuppressed(closing);
                }
                throw cause;
            }
        }
    }

    /** Keeps the implementation and its loader alive until the native handle closes. */
    public static final class FilterHandle implements AutoCloseable {
        private final Kind kind;
        private final URLClassLoader loader;
        private Object filter;

        private FilterHandle(Kind kind, Object filter, URLClassLoader loader) {
            this.kind = kind;
            this.filter = filter;
            this.loader = loader;
        }

        public EventData process(EventData event) {
            Object instance = Objects.requireNonNull(filter, "Filter is closed");
            return switch (kind) {
                case GENERATOR, UNSAFE_GENERATOR ->
                    Objects.requireNonNull(((Generator) instance).generate(), "Generator returned null");
                case CONVERTER, UNSAFE_CONVERTER ->
                    Objects.requireNonNull(((Converter) instance).convert(event), "Converter returned null");
                case WRITER -> {
                    ((Writer) instance).write(event);
                    yield null;
                }
            };
        }

        @Override
        public void close() throws Exception {
            Object instance = filter;
            if (instance == null) {
                return;
            }
            filter = null;
            // Close the loader even if user cleanup fails, preserving both exceptions.
            try (var resource = loader) {
                if (instance instanceof AutoCloseable closeable) {
                    closeable.close();
                }
            }
        }
    }
}
