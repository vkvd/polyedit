#pragma once
#include "stdafx.h"
#include "engine.h"

class Engine;

class State {
public:
	State(Engine* engine);
	~State();

	// Virtuals for each state's game loop
	virtual int enter()                                      = 0;
	virtual void exit()                                      = 0;
	virtual void update(double delta)                        = 0;
	virtual void draw(double delta)                          = 0;
	virtual void handleEvents(sf::Event event, double delta) = 0;

	// changeState that all states need
	void changeState(int state);

	// Main engine pointer for accessing other states
	Engine* m_engine;
};

