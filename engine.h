#pragma once
#include "stdafx.h"
#include "poly.h"
#include "point.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include "undoaction.h"

class Engine {
public:
	Engine(int aaLevel);
	~Engine();

	// Member functions
	// ------------------------

	void run();
	void handleEvents(sf::Event event); 
	void update();			            
	void draw();                        
	int  load();         
	
	void handleCamera();                
	void smoothnessToggle();            
	void clearSelection();              
	void deleteSelection();
	void onLeftClick(sf::Vector2f point);
	void onRightClick(sf::Vector2f point);
	void onMiddleClick(sf::Vector2f point);
	void undo();
	
	void saveJSON();
	void loadJSON();
	void saveVector(std::string filename); // save vector image

	sf::Vector2f getMPosFloat();
	sf::Vector2f windowToGlobalPos(const sf::Vector2f& vec);
	sf::Vector2f globalToWindowPos(const sf::Vector2f& vec);
	sf::Color    avgClr(Point& p1, Point& p2, Point& p3, int samples); 
	sf::Vector2f randPt(Point& p1, Point& p2, Point& p3); 
	sf::Vector2f getClampedImgPoint(const sf::Vector2f& vec);

	void createColorPickerGUI();
	void createSettingsGUI();
	void createHelpGUI();
	void handleGUItoggleEvent(sf::Event);
	// Members
	// -------------------------
	
	// Main engine window.
	sf::RenderWindow* window;

	// Background Color
	sf::Color BGColor;

	// Filenames: file is the image to load, vfile is the JSON, sfile is the SVG.
	std::string file;                   
	std::string vfile;                  
	std::string sfile;                  
         
	// Image data: image is the raw texture, img is a image to get pixel data from, drawimg is the drawable
	sf::Texture image;                  
	sf::Image   img;                      
	sf::Sprite  drawimg;                 

	// The view used for camera controls
	sf::View view;                       

	// Vectors: 
    // polygons: All polygons to render
	// rpoints: All drawable points to render
	// spoints: Pointers to rpoints that are selected
	// spointsin: Indices to rpoints corresponding to spoints; rpoints[spointsin] = spoints.
	// nspoints: Non-selected points.
	std::vector<Poly>   polygons;         
	std::vector<Point>  rpoints;         
	//std::vector<Point*> spoints;        
	std::vector<int>    spointsin;        
	std::vector<Point*> nspoints;       

	// Selected pointers:
	// spoint: Selected point for dragging.
	// spoly: it actually worked the whole time
	// spolycolor: Color of selected polygon in float[4]
	Poly*  spoly = NULL;
	Point* spoint;

	// Toggles:
	// Toggles for the drawing. Most are exactly what they are named.
	bool imgsmooth    = false;             
	bool wireframe    = false;
	bool wireframels  = false;
	bool showrvectors = true;          
	bool hideimage    = false;            
	bool showcenters  = false;
	// Not worth making another class.
	sf::Color centerdrawcolor;
	sf::Color pointIdleColor;
	sf::Color pointSelectedColor;
	// Zooming and dragging values:
	// viewzoom: Current scale of the view
	// dragflag: True if a point is being dragged.
	// vdragflag: True if the view is being dragged.
	// Offsets and initpts; Initpt is the initial point for dragging, the offset is calculated with it.
	// cmpos: Current mouse position for dragging.
	// nindex; Index of the point snapped to
	float viewzoom = 1.0f;              
	bool dragflag = false;              
	bool vdragflag = false;             
	sf::Vector2f vdragoffset;           
	sf::Vector2f vdraginitpt;           
	sf::Vector2f pdragoffset;           
	sf::Vector2f pdraginitpt;           
	sf::Vector2f cmpos;                 
	int nindex;             

	// GUI flags
	bool showColorPickerGUI = false;
	bool showSettingsGUI = false;
	bool showHelp = false;

	std::vector<UndoAction> undoBuffer;

	template <size_t S> class Sizer { };
};

