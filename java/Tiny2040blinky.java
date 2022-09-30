import pico.*;
import pimoroni.*;

@board(BoardType.Tiny2040)
class Tiny2040blinky
{
	static int RED_LED = tiny2040.LED_R_PIN;
	static int GREEN_LED = tiny2040.LED_G_PIN;
	static int BLUE_LED = tiny2040.LED_B_PIN;
	static int IRQ_PIN = 23;
	static int counter = 0;

	public static void main(String[] args)
	{
		stdio.init_all();

		gpio.init(RED_LED);
		gpio.set_dir(RED_LED, gpio.OUTPUT);
		gpio.put(RED_LED, 0);

		gpio.init(GREEN_LED);
		gpio.set_dir(GREEN_LED, gpio.OUTPUT);
		gpio.put(GREEN_LED, 0);

		gpio.init(BLUE_LED);
		gpio.set_dir(BLUE_LED, gpio.OUTPUT);
		gpio.put(BLUE_LED, 0);

		gpio.pull_down(IRQ_PIN);
		gpio.set_input_enabled(IRQ_PIN, true);
		gpio.set_irq_enabled_with_callback(IRQ_PIN, gpio.IRQ_EDGE_RISE, true, Tiny2040blinky::gpio_irq_callback);

		while (true)
		{
			time.sleep_ms(1000);
		}
	}

	static void gpio_irq_callback(int pin, int events)
	{
		if (events == gpio.IRQ_EDGE_RISE)
		{
			updateColour();
		}
	}

	static void updateColour()
	{
		counter = (counter + 1) % 8;

		gpio.put(RED_LED, counter & 1);
		gpio.put(GREEN_LED, counter & 2);
		gpio.put(BLUE_LED, counter & 4);
	}
}
