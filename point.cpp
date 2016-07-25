#include "stdafx.h"
#include "point.h"

Point::Point(){
	size = 5;
	vector = sf::Vector2f(0, 0);
	colorIdle = sf::Color::Green;
	colorSelected = sf::Color::Blue;
}
Point::Point(sf::Vector2f v, float s) {
	size = s;
	vector = v;
	colorIdle = sf::Color::Green;
	colorSelected = sf::Color::Blue;
}


Point::~Point()
{
}

void Point::updateCShape(float zoom){
	cshape.setRadius(size*zoom);
	sf::Vector2f pos = vector;
	pos.x -= size*zoom;
	pos.y -= size*zoom;
	cshape.setPosition(pos);
	cshape.setFillColor(sf::Color::Transparent);
	selected ? cshape.setOutlineColor(colorSelected) : cshape.setOutlineColor(colorIdle);
	cshape.setOutlineThickness(-1.5*zoom);
}

bool Point::operator==(const Point& rhs){
	if (vector == rhs.vector){
		return true;
	}
	else {
		return false;
	}
}
