package org.cola.test;

import org.cola.api.Converter;
import org.cola.api.EventData;
import org.cola.api.internal.Bridge;

/** Runs directly in Java, without the C++ calculation host. */
public final class BridgeTest {
    private static int closed;

    public static final class ClosingConverter implements Converter, AutoCloseable {
        public EventData convert(EventData event) {
            return event;
        }

        public void close() {
            closed++;
            throw new IllegalStateException("close failure");
        }
    }

    public static final class BadConstructor implements Converter {
        public BadConstructor() {
            throw new IllegalArgumentException("constructor failure");
        }

        public EventData convert(EventData event) {
            return event;
        }
    }

    public static final class NullConverter implements Converter {
        public EventData convert(EventData event) {
            return null;
        }
    }

    private static Bridge.FilterLoader loader(String jar, String type) {
        var loader = new Bridge.FilterLoader();
        loader.putMetadata("jar", jar);
        loader.putMetadata("class", type);
        return loader;
    }

    @FunctionalInterface
    private interface Action {
        void run() throws Throwable;
    }

    private static void fails(Class<? extends Throwable> type, String message, Action action)
            throws Throwable {
        try {
            action.run();
        } catch (Throwable error) {
            if (!type.isInstance(error) || !error.getMessage().contains(message)) {
                throw new AssertionError("Unexpected exception", error);
            }
            return;
        }
        throw new AssertionError("Expected " + type.getName());
    }

    public static void main(String[] args) throws Throwable {
        Bridge.loadNativeLibrary(args[0]);
        String jar = args[1];
        fails(IllegalArgumentException.class, "class", () -> new Bridge.FilterLoader().load(0));
        fails(ClassCastException.class, "", () -> loader(jar, ClosingConverter.class.getName()).load(0));
        fails(IllegalArgumentException.class, "constructor failure",
                () -> loader(jar, BadConstructor.class.getName()).load(1));

        var event = new EventData();
        try {
            var filter = loader(jar, ClosingConverter.class.getName()).load(1);
            if (filter.process(event) != event) {
                throw new AssertionError("Bridge changed event identity");
            }
            fails(IllegalStateException.class, "close failure", filter::close);
            filter.close();
            if (closed != 1) {
                throw new AssertionError("Filter must close exactly once");
            }
            fails(NullPointerException.class, "Filter is closed", () -> filter.process(event));
            try (var nullFilter = loader(jar, NullConverter.class.getName()).load(1)) {
                fails(NullPointerException.class, "Converter returned null", () -> nullFilter.process(event));
            }
        } finally {
            event.delete();
        }
        String trace = Bridge.formatException(new IllegalStateException("diagnostic"));
        if (!trace.contains("diagnostic") || !trace.contains("BridgeTest.main")) {
            throw new AssertionError("Missing Java stack trace");
        }
    }
}
