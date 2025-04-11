#include <SFML/Graphics.hpp>

#include "Game.h" 
#include <iostream>

int main()
{

	Vec2 v1(6, 9);
	Vec2 v2(2, 4);

	Vec2 normalized = v1.normalize();
	std::cout << normalized.x << " " << normalized.y;

	Game g("config.txt");
	g.run();
}