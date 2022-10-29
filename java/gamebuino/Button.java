package gamebuino;

public class Button
{
	public static int A;
	public static int B;
	public static int LEFT;
	public static int RIGHT;
	public static int UP;
	public static int DOWN;
	public static int MENU;

	public native boolean repeat(int button, int period);
	public native boolean pressed(int button);
	public native boolean released(int button);
	public native boolean held(int button, int time);
	public native int timeHeld(int button);
}
