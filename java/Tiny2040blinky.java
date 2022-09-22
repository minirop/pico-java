import pico.*;

@board(BoardType.Tiny2040)
class Tiny2040blinky
{
	static int RED_LED = 18;
	static int GREEN_LED = 19;
	static int BLUE_LED = 20;

	public static void main(String[] args)
	{
		stdio.init_all();

		gpio.init(RED_LED);
		gpio.set_dir(RED_LED, gpio.OUTPUT);
		gpio.put(RED_LED, 1);

		gpio.init(GREEN_LED);
		gpio.set_dir(GREEN_LED, gpio.OUTPUT);
		gpio.put(GREEN_LED, 1);

		gpio.init(BLUE_LED);
		gpio.set_dir(BLUE_LED, gpio.OUTPUT);
		gpio.put(BLUE_LED, 1);

		int counter = 0;

		while (true)
		{
			gpio.put(RED_LED, counter & 1);
			gpio.put(GREEN_LED, counter & 2);
			gpio.put(BLUE_LED, counter & 4);
			time.sleep_ms(1000);

			counter = (counter + 1) % 8;
		}
	}
}
