package org.cola.example

import org.cola.api.Converter
import org.cola.api.EventData

class MomentumScaler(metadata: Map<String, String>) : Converter {
    private val scale = metadata["scale"]?.toDouble() ?: 1.0

    override fun convert(event: EventData): EventData {
        event.particles.forEach { particle ->
            particle.momentum.apply {
                e *= scale
                x *= scale
                y *= scale
                z *= scale
            }
        }
        return event
    }
}
