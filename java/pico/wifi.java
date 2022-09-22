package pico;

public class wifi
{
	public static int LED_PIN = 0;
	
	public static native void init();
	public static native void gpio_put(int pin, int value);
}
