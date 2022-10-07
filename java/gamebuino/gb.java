package gamebuino;

public class gb
{
	public static native void begin();
	public static native void waitForUpdate();
	public static native void setFrameRate(int fps);

	public static Image display;
	public static Button buttons;

	public static int frameCount;
}
