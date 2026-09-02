package org.cola.test;

import org.cola.api.EventData;
import org.cola.api.UnsafeConverter;

public final class TestUnsafeConverter implements UnsafeConverter {
    private EventData previous;

    @Override
    public EventData convert(EventData event) {
        if (previous != null && previous.isValid()) {
            throw new IllegalStateException("Unsafe converter output still owns its native event");
        }
        event.getInitialState().setEnergy(17.0);
        previous = event;
        return event;
    }
}
