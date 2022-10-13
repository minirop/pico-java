package pimoroni;

import types.*;

public class picosystem
{
	public static @unsigned int HFLIP = 0x01;
	public static @unsigned int VFLIP = 0x02;

	public static native void blit(@pointer buffer src, int x, int y, int w, int h, int dx, int dy);
	public static native void blit(@pointer buffer src, int x, int y, int w, int h, int dx, int dy, int flags);
	public static native void blit(@pointer buffer src, int x, int y, int w, int h, int dx, int dy, int dw, int dh);
	public static native void blit(@pointer buffer src, int x, int y, int w, int h, int dx, int dy, int dw, int dh, int flags);

	public static native void clear();

	public static int UP;
	public static int DOWN;
	public static int LEFT;
	public static int RIGHT;
	public static int A;
	public static int B;
	public static int X;
	public static int Y;
	public static native boolean button(int button);
}
