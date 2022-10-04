import board.*;
import pico.*;
import pimoroni.*;

@Board(Type.Tiny2040)
class ButtonBlinky
{
	public static int IRQ_PIN = 3;
	public static int RED_LED = tiny2040.LED_R_PIN;
	public static int GREEN_LED = tiny2040.LED_G_PIN;
	public static int BLUE_LED = tiny2040.LED_B_PIN;
	public static int RED_LED_2 = 7;
	public static int GREEN_LED_2 = 6;

	public static void main(String[] args)
	{
		stdio.init_all();

		gpio.init(IRQ_PIN);
		gpio.set_dir(IRQ_PIN, gpio.INPUT);
		gpio.pull_up(IRQ_PIN);
		gpio.set_input_enabled(IRQ_PIN, true);
		gpio.set_irq_enabled_with_callback(IRQ_PIN, gpio.IRQ_EDGE_RISE, true, ButtonBlinky::gpio_irq_callback);

		gpio.init(RED_LED);
		gpio.set_dir(RED_LED, gpio.OUTPUT);
		gpio.init(GREEN_LED);
		gpio.set_dir(GREEN_LED, gpio.OUTPUT);
		gpio.init(BLUE_LED);
		gpio.set_dir(BLUE_LED, gpio.OUTPUT);
		gpio.init(RED_LED_2);
		gpio.set_dir(RED_LED_2, gpio.OUTPUT);
		gpio.init(GREEN_LED_2);
		gpio.set_dir(GREEN_LED_2, gpio.OUTPUT);

		gpio.put(BLUE_LED, 1);

		gpio_irq_callback(IRQ_PIN, gpio.IRQ_EDGE_RISE);

		while (true)
		{
			time.sleep_ms(1000);
		}
	}

	static int i = 0;
	static void gpio_irq_callback(int pin, int events)
	{
		if (events == gpio.IRQ_EDGE_RISE)
		{
			int k = i;
			i = (i + 1) % 2;

			gpio.put(GREEN_LED, i);
			gpio.put(RED_LED, k);
			gpio.put(GREEN_LED_2, k);
			gpio.put(RED_LED_2, i);
		}
	}
}
