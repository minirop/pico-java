package gamebuino;

import types.*;

public class gb
{
	public static native void begin();
	public static native boolean update();
	public static native void waitForUpdate();
	public static native void setFrameRate(@unsigned byte fps);
	public static native @unsigned byte getCpuLoad();
	public static native @unsigned short getFreeRam();
	public static native Rotation getScreenRotation();
	public static native void setScreenRotation(Rotation r);

	public static Image display;
	public static Button buttons;
	public static Sound sound;
	public static Save save;
	public static Gui gui;

	public static int frameCount;
}
