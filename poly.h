#pragma once
#include "stdafx.h"
#include "point.h"

class Poly {
public:
	Poly();
	Poly(Point* _p1, Point* _p2, Point* _p3, int _s1, int _s2, int _s3, sf::Color _fillcolor);
	~Poly();

	// Methods

	// Update the drawable's coordinates to the poly's internal coords 
	void updateCShape(float viewzoom);
	// Changes p1-3 to rpoints[rpointIndices[1-3]]
	void updatePointers(std::vector<Point>& pts);
	// Prints all points in the poly formatted
	void dbgPrintAllPoints();
	// Recalculates center - useful for dragging
	void updateCenter();

	// Members

	// List of indices in rpoints that correlates to p1-3
	int rpointIndices[3];
	// Polygon points
	Point* p1;
	Point* p2;
	Point* p3;
	sf::Vector2f center;
	sf::Color fillcolor;
	// Drawable shape
	sf::ConvexShape cshape;
	bool isWireframe = false;
	bool selected = false;
};

