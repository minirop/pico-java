package pico;

public class gpio
{
	public static int INPUT = 0;
	public static int OUTPUT = 1;

	public static native void init(int pin);
	public static native void set_dir(int pin, int dir);
	public static native void put(int pin, int value);
	public static native void pull_up(int pin);
	public static native boolean get(int pin);
	public static native void set_mask(int mask);
	public static native void clr_mask(int mask);
}
