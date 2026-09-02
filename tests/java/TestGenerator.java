package org.cola.test;

import org.cola.api.EventData;
import org.cola.api.Generator;
import org.cola.api.LorentzVector;
import org.cola.api.Particle;
import org.cola.api.ParticleClass;

public final class TestGenerator implements Generator {
    private EventData previous;

    @Override
    public EventData generate() {
        if (previous != null && previous.getInitialState().getEnergy() != 42.0) {
            throw new IllegalStateException("Previously returned generator event is no longer valid");
        }
        var event = new EventData();
        event.getInitialState().setEnergy(42.0);

        var particle = new Particle();
        particle.setPosition(new LorentzVector(0.0, 0.0, 0.0, 0.0));
        particle.setMomentum(new LorentzVector(5.0, 1.0, 2.0, 3.0));
        particle.setPdgCode(211);
        particle.setParticleClass(ParticleClass.PRODUCED);
        event.getParticles().add(particle);
        previous = event;
        return event;
    }
}
