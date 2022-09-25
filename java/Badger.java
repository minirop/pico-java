import pico.*;
import pimoroni.*;

@board(BoardType.Badger2040)
class Badger
{
	public static void main(String[] args)
	{
		badger.init();

		String button = "";
		String message = "started up.";
		if(badger.pressed_to_wake(badger.A)) { button += "A"; }
		if(badger.pressed_to_wake(badger.B)) { button += "B"; }
		if(badger.pressed_to_wake(badger.C)) { button += "C"; }
		if(badger.pressed_to_wake(badger.D)) { button += "D"; }
		if(badger.pressed_to_wake(badger.E)) { button += "E"; }

		if(button != "")
		{
			message = "woken up by button " + button + ".";
		}
		
		badger.thickness(2);

		badger.pen(15);
		badger.clear();
		badger.pen(0);
		badger.text(message, 10, 20, 0.6f);
		badger.text("(press any button to go to sleep.)", 10, 70, 0.4f);
		badger.update();

		badger.wait_for_press();

		badger.pen(15);
		badger.clear();
		badger.pen(0);
		badger.text("going back to sleep...", 10, 20, 0.6f);
		badger.text("z", 220, 50, 0.6f);
		badger.text("z", 230, 40, 0.8f);
		badger.text("z", 240, 30, 1.0f);
		badger.text("(press any button to wake up.)", 10, 70, 0.4f);
		badger.update();

		while (badger.is_busy())
		{
			time.sleep_ms(10);
		}

		badger.halt();
		// proof we halted, the LED will now turn on
		badger.led(255);
	}
}
