// conversion from the game made by OptimusVenatus:
// https://gamebuino.com/creations/powercircle

import board.*;
import gamebuino.*;
import arduino.*;

@Board(Type.Gamebuino)
class PowerCircle
{
	static int state = 0;
	static int xsize;
	static int ysize;
	static float objective;
	static float position = 0;
	static float speed = 1;
	static int score = 0;
	static int way = 1;
	static int hb = 0;
	static int highscore = 0;

	static void reset()
	{
		objective = std.random(0, (int)((2 * std.PI) * 100)) / 100;
		speed = 1;
		score = 0;
		way = 1;
	}

	static void setup()
	{
		gb.begin();
		gb.save.set(0, score);
		gb.setScreenRotation(Rotation.right);
		xsize = gb.display.width();
		ysize = gb.display.height();
		objective = std.random(0, (int)((2 * std.PI) * 100)) / 100;
		highscore = gb.save.get(0);
	}

	static void loop()
	{
		if (state == 0)
		{
			game();
		}
		else if (state == 1)
		{
			gameOver();
		}
		/*
		switch (state)
		{
		case 0:
			game();
			break;
		case 1:
			gameOver();
			break;
		default:
			break;
		}
		*/
	}

	static void gameOver()
	{
		while (!gb.update());
		gb.display.clear();

		gb.display.setColor(Color.beige);
		gb.display.setFontSize(1);
		String text = "GAME OVER";
		gb.display.setCursor((xsize / 2) - (gb.display.getFontWidth() * text.length() / 2), ysize / 2 - 20);
		gb.display.print(text);

		gb.display.setColor(Color.red);
		text = "press A";
		gb.display.setFontSize(1);
		gb.display.setCursor((xsize / 2) - ((gb.display.getFontWidth()) * text.length() / 2), ysize - gb.display.getFontHeight());
		gb.display.print(text);

		gb.display.setColor(Color.gray);
		if (hb == 1)
		{
			gb.display.setColor(Color.green);
		}
		text = "Score : " + String.valueOf(score);
		gb.display.setFontSize(1);
		gb.display.setCursor((xsize / 2) - (gb.display.getFontWidth() * text.length() / 2), ysize / 2);
		gb.display.print(text);

		text = "Best Score : " + String.valueOf(highscore);
		gb.display.setFontSize(1);
		gb.display.setCursor((xsize / 2) - (gb.display.getFontWidth() * text.length() / 2), ysize / 2 + 20);
		gb.display.print(text);

		if (gb.buttons.pressed(Button.A))
		{
			state = 0;
			gb.sound.playOK();
			reset();
		}
	}

	static void game()
	{
		while (!gb.update());
		gb.display.clear();

		position = position + (0.05f * speed) * way;
		gb.display.setColor(Color.beige);
		gb.display.fillCircle(xsize / 2, ysize / 2, xsize / 2);
		gb.display.setColor(Color.black);
		gb.display.fillCircle(xsize / 2, ysize / 2, xsize / 4);

		gb.display.setColor(Color.red);
		gb.display.fillCircle(xsize / 2 + (int)(std.cos(objective) * xsize * 0.375), ysize / 2 + (int)(std.sin(objective) * xsize * 0.375), xsize / 8);

		if (gb.buttons.pressed(Button.A))
		{
			if (gb.display.getPixelColor(xsize / 2 + (int)(std.cos(position) * xsize * 0.375), ysize / 2 + (int)(std.sin(position) * xsize * 0.375)) == Color.red)
			{
				score++;
				objective = std.random(0, (int)((2 * std.PI) * 300)) / 300;
				way = way * -1;
				gb.sound.playOK();
				speed = speed + 0.1f;
			}
			else
			{
				state = 1;
				hb = 0;
				highscore = gb.save.get(0);
				if (score > highscore)
				{
					gb.save.set(0, score);
					highscore = score;
					gb.gui.popup("New Highscore!", 50);
					hb = 1;
				}
				gb.sound.playCancel();
			}
		}

		gb.display.setColor(Color.gray);
		gb.display.setCursor(xsize / 2 - 5, ysize / 2 - 5);
		gb.display.print(score);

		gb.display.setColor(Color.blue);
		gb.display.drawLine(xsize / 2 + (int)(std.cos(position) * xsize * 0.25), ysize / 2 + (int)(std.sin(position) * xsize * 0.25), xsize / 2 + (int)(std.cos(position) * xsize * 0.5), ysize / 2 + (int)(std.sin(position) * xsize * 0.5));
	}
}
