package pimoroni;

public class badger
{
	public static int A;
	public static int B;
	public static int C;
	public static int D;
	public static int E;
	public static int UP;
	public static int DOWN;
	public static int USER;
	public static int CS;
	public static int CLK;
	public static int MOSI;
	public static int DC;
	public static int RESET;
	public static int BUSY;
	public static int VBUS_DETECT;
	public static int LED;
	public static int BATTERY;
	public static int ENABLE_3V3;

	public static native void init();
	public static native void update();
	public static native void halt();
	public static native void clear();
	public static native void wait_for_press();
	public static native void led(int brightness);
	public static native boolean is_busy();
	public static native void text(String string, int x, int y, float scale);
	public static native boolean pressed_to_wake(int pin);
	public static native void thickness(int size);
	public static native void pen(int colour);
}
