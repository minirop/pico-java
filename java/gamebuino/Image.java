package gamebuino;

import types.*;

public class Image
{
	public Image(byte[] data) {}
	public Image(short[] data) {}
	public Image(String string, Format format) {}
	public Image(String string, Format format, int yframes) {}
	public Image(String string, Format format, int yframes, int xframes) {}
	public Image(String string, Format format, int yframes, int xframes, int loop) {}

	public native void clear();
	public native void print(int number);
	public native void print(String text);
	public native void println(String text);
	public native void printf(String format, Object... args);

	public native void drawImage(int x, int y, Image img);
	public native void drawImage(int x, int y, Image img, int width, int height);
	public native void drawImage(int x, int y, Image img, int src_x, int src_y, int src_w, int src_h);
	public native void setFrame(@unsigned int frame);

	public native void fillRect(int x, int y, int width, int height);

	public native void setCursor(int x, int y);

	public native int width();
	public native int height();
}
