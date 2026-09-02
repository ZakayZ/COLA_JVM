package org.cola.api;

/**
 * A zero-copy generator whose returned event is forcefully transferred to COLA.
 *
 * <p>The returned {@link EventData} proxy becomes invalid as soon as {@link #generate()} returns.
 */
public interface UnsafeGenerator extends Generator {}
