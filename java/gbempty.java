import board.*;
import gamebuino.*;

@Board(Type.Gamebuino)
class gbempty
{
    static void setup()
    {
        gb.begin();
    }

    static void loop()
    {
        gb.waitForUpdate();

        gb.display.clear();
    }
}
