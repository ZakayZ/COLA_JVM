package org.cola.test;

import org.cola.api.EventData;
import org.cola.api.Writer;

public final class TestWriter implements Writer {
    private EventData previous;

    @Override
    public void write(EventData event) {
        if (event.getInitialState() == null) {
            throw new IllegalStateException("Writer received an empty EventData proxy");
        }
        if (previous != null && previous.getInitialState() == null) {
            throw new IllegalStateException("Previously received writer event is no longer valid");
        }
        previous = event;
    }
}
