package pico;

import java.util.function.BiConsumer;

public class gpio
{
	public static int INPUT;
	public static int OUTPUT;
	public static int IRQ_EDGE_RISE;
	public static int IRQ_EDGE_FALL;

	public static native void init(int pin);
	public static native void set_dir(int pin, int dir);
	public static native void put(int pin, int value);
	public static native void pull_up(int pin);
	public static native void pull_down(int pin);
	public static native void set_input_enabled(int pin, boolean enabled);
	public static native void set_irq_enabled_with_callback(int pin, int events, boolean enabled, BiConsumer<Integer, Integer> callback);
	public static native boolean get(int pin);
	public static native void set_mask(int mask);
	public static native void clr_mask(int mask);
}
