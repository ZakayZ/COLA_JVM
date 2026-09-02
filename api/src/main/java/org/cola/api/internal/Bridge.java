package org.cola.api.internal;

import java.net.URL;
import java.net.URLClassLoader;
import java.nio.file.Path;
import java.util.HashMap;
import java.util.Map;
import org.cola.api.Converter;
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

    public static final class FilterLoader {
        private final String jarPath;
        private final String className;
        private final Map<String, String> metadata = new HashMap<>();

        public FilterLoader(String jarPath, String className) {
            this.jarPath = jarPath;
            this.className = className;
        }

        public void putMetadata(String key, String value) {
            metadata.put(key, value);
        }

        public Generator loadGenerator() throws Exception {
            return load(Generator.class);
        }

        public Converter loadConverter() throws Exception {
            return load(Converter.class);
        }

        public UnsafeGenerator loadUnsafeGenerator() throws Exception {
            return load(UnsafeGenerator.class);
        }

        public UnsafeConverter loadUnsafeConverter() throws Exception {
            return load(UnsafeConverter.class);
        }

        public Writer loadWriter() throws Exception {
            return load(Writer.class);
        }

        private <T> T load(Class<T> interfaceType) throws Exception {
            return Bridge.load(jarPath, className, metadata, interfaceType);
        }
    }

    private static <T> T load(
            String jarPath, String className, Map<String, String> metadata, Class<T> interfaceType)
            throws Exception {
        Class<?> implementation =
                Class.forName(
                        className,
                        true,
                        new URLClassLoader(
                                new URL[] {Path.of(jarPath).toAbsolutePath().toUri().toURL()},
                                Bridge.class.getClassLoader()));
        try {
            return interfaceType.cast(implementation.getConstructor(Map.class).newInstance(metadata));
        } catch (NoSuchMethodException ignored) {
            return interfaceType.cast(implementation.getConstructor().newInstance());
        }
    }
}
