#pragma once
#include "stdafx.h"
#include <iostream>
#include "point.h"

enum class Action
{
	pointDeletion,
	pointAddition,
	polyDeletion,
	polyAddition,
	polyRecolor
};

struct PD
{
	std::vector<std::pair<Point,int>> deletedPoints;
    std::vector<std::tuple<int,int,int>> cPointPolysIndices;
	std::vector<sf::Color> cPointColors;
};

struct PA
{
	int pointIndex;	
};

struct pD
{
	int pointIndices[3];
	sf::Color polyColor;
};

struct pA
{
	int polyIndex;
};

struct pR
{
	int polyIndex;
	sf::Color polyColor;
};

class UndoAction
{
public:
	UndoAction();
	UndoAction(PD a) : pointDeletion(a), action(Action::pointDeletion) {}
	UndoAction(PA a) : pointAddition(a), action(Action::pointAddition) {}
	UndoAction(pD a) : polyDeletion (a), action(Action::polyDeletion)  {}
	UndoAction(pA a) : polyAddition (a), action(Action::polyAddition)  {}
	UndoAction(pR a) : polyRecolor  (a), action(Action::polyRecolor)   {}
	~UndoAction();
	Action action;
	PA pointAddition;
	PD pointDeletion;
	pA polyAddition;
	pD polyDeletion;
	pR polyRecolor;
	void print() {
		switch ((int)action) {
		case 0:
			std::cout << "Point Deletion:\n";
			std::cout << "\tPoints: "<< pointDeletion.deletedPoints.size()<<"\n";
			std::cout << "\t\tIndices: ";
			for (auto e : pointDeletion.deletedPoints) {
				std::cout << "(" << e.second << ") ";
			}
			std::cout << "\tPolygons destroyed: " << pointDeletion.cPointPolysIndices.size() << "\n";
			for (auto e : pointDeletion.cPointPolysIndices) {
				std::cout << "\t\t" << std::get<0>(e) << " " << std::get<1>(e) << " " << std::get<2>(e) << "\n";
			}
			break;
		case 1:
			std::cout << "Point Addition:\n";
			std::cout << "\tIndex: (" << pointAddition.pointIndex << ")\n";
			break;
		case 2:
			break;
		case 3:
			std::cout << "Poly Addition\n";
			std::cout << "\tIndex: (" << polyAddition.polyIndex << ")\n";
			break;
		case 4:
			std::cout << "Poly Recolor\n";
			std::cout << "\tIndex: (" << polyRecolor.polyIndex << ")\n";
			std::cout << "\tColor: (" << polyRecolor.polyColor.toInteger() << ")\n";
			break;
		default:
			std::cout << "invalid type: something went VERY wrong, please save and restart\n";
		}
	}
};

