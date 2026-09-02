package org.cola.api;

/**
 * A zero-copy converter whose returned event is forcefully transferred to COLA.
 *
 * <p>The returned {@link EventData} proxy becomes invalid as soon as {@link #convert(EventData)}
 * returns. This includes the input proxy when the converter returns its input.
 */
public interface UnsafeConverter extends Converter {}
