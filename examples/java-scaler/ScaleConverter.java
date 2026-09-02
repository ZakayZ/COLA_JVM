package org.cola.example;

import java.util.Map;
import org.cola.api.Converter;
import org.cola.api.EventData;

public final class ScaleConverter implements Converter {
    private final double scale;

    public ScaleConverter(Map<String, String> metadata) {
        scale = Double.parseDouble(metadata.getOrDefault("scale", "2.0"));
    }

    @Override
    public EventData convert(EventData event) {
        event.getParticles().forEach(particle -> {
            var momentum = particle.getMomentum();
            momentum.setE(momentum.getE() * scale);
            momentum.setX(momentum.getX() * scale);
            momentum.setY(momentum.getY() * scale);
            momentum.setZ(momentum.getZ() * scale);
        });
        return event;
    }
}
