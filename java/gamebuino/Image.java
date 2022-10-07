package gamebuino;

import types.*;

public class Image
{
	public Image(byte[] data) {}
	public Image(short[] data) {}
	public Image(String string, Format format) {}
	public native void clear();
	public native void print(String text);
	public native void println(String text);
	public native void printf(String format, Object... args);

	public native void drawImage(int x, int y, Image img);
	public native void drawImage(int x, int y, Image img, int width, int height);
	public native void setFrame(int frame);
}
