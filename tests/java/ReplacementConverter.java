package org.cola.test;

import org.cola.api.Converter;
import org.cola.api.EventData;

public final class ReplacementConverter implements Converter {
    private EventData previousInput;
    private EventData previousOutput;

    @Override
    public EventData convert(EventData input) {
        if (previousInput != null && previousInput.getParticles().isEmpty()) {
            throw new IllegalStateException("Previously received converter input is no longer valid");
        }
        if (previousOutput != null && previousOutput.getInitialState().getEnergy() != 77.0) {
            throw new IllegalStateException("Previously returned converter output is no longer valid");
        }
        var replacement = new EventData();
        replacement.getInitialState().setEnergy(77.0);
        previousInput = input;
        previousOutput = replacement;
        return replacement;
    }
}
