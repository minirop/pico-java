package pimoroni;

public @interface Picosystem
{
	boolean DoublePixels() default false;
	boolean StartupLogo() default true;
	boolean SpriteSheet() default true;
	boolean Font() default true;
	boolean Overclock() default true;
}
