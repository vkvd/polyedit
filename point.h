#pragma once
#include "SFML/Graphics.hpp"

class Point
{
public:
	Point();
	Point(sf::Vector2f v, float s);
	~Point();
	sf::Vector2f vector;
	float size;
	sf::CircleShape cshape;
	sf::Color colorIdle;
	sf::Color colorSelected;
	bool selected = false;
	void updateCShape(float zoom);
	bool operator==(const Point& rhs);
};

