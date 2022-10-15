package gamebuino;

import types.*;

public class Save
{
	public native int get(@unsigned int block);
	public native boolean set(@unsigned int block, int number);
}
