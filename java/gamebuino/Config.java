package gamebuino;

public @interface Config
{
	public static int DISPLAY_MODE_RGB565 = 0;
	public static int DISPLAY_MODE_INDEX = 1;
	public static int DISPLAY_MODE_INDEX_HALFRES = 2;
	int DisplayMode() default -1;
	int FontSize() default -1;
}
