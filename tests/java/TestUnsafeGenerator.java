package org.cola.test;

import org.cola.api.EventData;
import org.cola.api.UnsafeGenerator;

public final class TestUnsafeGenerator implements UnsafeGenerator {
    private EventData previous;

    @Override
    public EventData generate() {
        if (previous != null && previous.isValid()) {
            throw new IllegalStateException("Unsafe generator output still owns its native event");
        }
        var event = new EventData();
        event.getInitialState().setEnergy(13.0);
        previous = event;
        return event;
    }
}
