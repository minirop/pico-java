// conversion from the game made in this tutorial:
// https://gamebuino.com/academy/workshop/make-your-very-first-games-with-pong

import board.*;
import gamebuino.*;
import std.*;

@Board(Type.Gamebuino)
class Pong
{
	static int balle_posX = 20;
	static int balle_posY = 20;
	static int balle_speedX = 1;
	static int balle_speedY = 1;
	static int balle_taille = 3;
	static int raquette1_posX = 10;
	static int raquette1_posY = 30;
	static int raquette2_posX = gb.display.width() - 13;
	static int raquette2_posY = 30;
	static int raquette_hauteur = 10;
	static int raquette_largeur = 3;
	static int raquette2_speedY = 0;
	static int score1;
	static int score2;
	static int difficulte = 3;

	static void setup()
	{
		gb.begin();
	}

	static void loop()
	{
		while (!gb.update());
		gb.display.clear();

		if (gb.buttons.pressed(Button.MENU))
		{
			if (difficulte == 3)
			{ // Facile
				difficulte = 2;
			}
			else
			{  // Difficile
				difficulte = 3;
			}
		}

		if (gb.buttons.repeat(Button.UP, 0))
		{
			raquette1_posY = raquette1_posY - 1;
		}
		if (gb.buttons.repeat(Button.DOWN, 0))
		{
			raquette1_posY = raquette1_posY + 1;
		}

		if (balle_posY > raquette2_posY + raquette_hauteur / 2 && std.random(0, difficulte) == 1)
		{
			raquette2_speedY = 2;
		}
		else if (balle_posY < raquette2_posY + raquette_hauteur / 2 && std.random(0, difficulte) == 1)
		{
			raquette2_speedY = -2;
		}
		raquette2_posY = raquette2_posY + raquette2_speedY;  // Mettre à jour la position de la raquette2

		balle_posX = balle_posX + balle_speedX;
		balle_posY = balle_posY + balle_speedY;

		if (balle_posY < 0)
		{
			balle_speedY = 1;
		}
		if (balle_posY > gb.display.height() - balle_taille)
		{
			balle_speedY = -1;
		}

		if ( (balle_posX == raquette1_posX + raquette_largeur)
			&& (balle_posY + balle_taille >= raquette1_posY)
			&& (balle_posY <= raquette1_posY + raquette_hauteur) )
		{
			balle_speedX = 1;
		}

		if ( (balle_posX + balle_taille == raquette2_posX)
			&& (balle_posY + balle_taille >= raquette2_posY)
			&& (balle_posY <= raquette2_posY + raquette_hauteur) )
		{
			balle_speedX = -1;
		}

		if (balle_posX < 0)
		{
			balle_posX = 20;
			balle_posY = std.random(20, gb.display.height() - 20);  // Position aléatoire au centre de l'écran
			balle_speedX = 1;
			if (std.random(0, 2) == 1)
			{
				balle_speedY = 1;
			} 
			else
			{
				balle_speedY = -1;
			}

			score2 = score2 + 1;
		}

		if (balle_posX > gb.display.width())
		{
			balle_posX = 20;
			balle_posY = std.random(20, gb.display.height() - 20);  // Position aléatoire au centre de l'écran
			balle_speedX = 1;
			if (std.random(0, 2) == 1)
			{
				balle_speedY = 1;
			} 
			else
			{
				balle_speedY = -1;
			}

			score1 = score1 + 1;
		}

		gb.display.fillRect(balle_posX, balle_posY, balle_taille, balle_taille);
		gb.display.fillRect(raquette1_posX, raquette1_posY, raquette_largeur, raquette_hauteur);
		gb.display.fillRect(raquette2_posX, raquette2_posY, raquette_largeur, raquette_hauteur);

		gb.display.setCursor(35, 5);
		gb.display.print(score1);
		gb.display.setCursor(42, 5);
		gb.display.print(score2);

		gb.display.setCursor(33, gb.display.height() - 5);
		if (difficulte == 3)
		{
			gb.display.print("EASY");
		}
		else
		{
			gb.display.print("HARD");
		}
	}
}
