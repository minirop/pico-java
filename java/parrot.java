import board.*;
import types.*;
import pimoroni.*;

@Board(Type.Picosystem)
@Picosystem(DoublePixels = true, StartupLogo = false)
class parrot
{
	static @unsigned final short[] parrot_image = null;
	static buffer parrot = new buffer(120, 120, parrot_image);
	static int x = 0;
	static int y = 0;

	static void init()
	{
	}

	static void update(@unsigned int tick)
	{
		if (picosystem.button(picosystem.LEFT))
		{
			x -= 1;
		}
		else if (picosystem.button(picosystem.RIGHT))
		{
			x += 1;
		}
		if (picosystem.button(picosystem.UP))
		{
			y -= 1;
		}
		else if (picosystem.button(picosystem.DOWN))
		{
			y += 1;
		}
	}

	static void draw(@unsigned int tick)
	{
		int dx = x;
		int dy = y;
		int width = 120;
		int height = 120;

		if (x < 0)
		{
			width += x;
			dx = 0;
		}

		if (y < 0)
		{
			height += y;
			dy = 0;
		}

		picosystem.clear();
		picosystem.blit(parrot, 120-width, 120-height, width, height, dx, dy);
	}
}
