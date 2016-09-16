#include "stdafx.h"
#include "engine.h"
#include "poly.h"
#include "tinyfiledialogs.h"
#include "json/json.h"
#include <iomanip>
#include "imgui/imgui.h"
#include "imgui/imconfig.h"
#include "imgui-backends/SFML/imgui-events-SFML.h"
#include "imgui-backends/SFML/imgui-rendering-SFML.h"
#include "imgui/imguicolorpicker.h"

// Fixes for windows
#ifdef _WIN32
#define strdup _strdup
#define snprintf sprintf_s
//#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup") // Don't open console with window
#endif

// The title of the window set in the constructor.
#define WINDOWTITLE "Lowpoly Editor"
// Starting window size.
#define WINDOW_X 640
#define WINDOW_Y 480
// Fixed framerate set in the engine constructor.
#define FRAMERATE 144
// Range in pixels to snap to already existing points.
#define GRABDIST 10

// Number of decimals to care about when using randdbl.
// RPRECISION should equal 10^x where x is the relevant decimal count.
#define RPRECISION 10000
// Returns a random double between a and b.
#define randdbl(a,b) ((rand()%(RPRECISION*(b-a)))/(RPRECISION*((float)b-a))+a)

// Returns the distance between two vectors.
float v2fdistance(sf::Vector2f a, sf::Vector2f b){
	return std::sqrt((b.x - a.x)*(b.x - a.x) + (b.y - a.y)*(b.y - a.y));
}

// Constructor for the main engine.
// Sets up renderwindow variables and loads an image.
Engine::Engine(int aaLevel) {
	std::cout << sizeof(undoBuffer)/1024;
	BGColor = sf::Color(125, 125, 125, 255);
	sf::ContextSettings settings;
	settings.antialiasingLevel = aaLevel;
	window = new sf::RenderWindow(sf::VideoMode(WINDOW_X, WINDOW_Y), WINDOWTITLE, sf::Style::Default,settings);
	window->setFramerateLimit(FRAMERATE);
	window->setVerticalSyncEnabled(false);
	window->setKeyRepeatEnabled(false);
	if (load() != 0){
		std::exit(1);
	}
	view.reset(sf::FloatRect(0, 0, WINDOW_X, WINDOW_Y));
	window->setView(view);
	drawimg.setTexture(image);

	// Initialize GUI and backend
	ImGui::SFML::SetRenderTarget(*window);
	ImGui::SFML::InitImGuiRendering();
	ImGui::SFML::SetWindow(*window);
	ImGui::SFML::InitImGuiEvents();
}

// Destructor for the main engine.
// Deletes the window.
Engine::~Engine() {
	delete window;
}

// Main loop of the engine.
// Delegates events to the Engine::handleEvents() function,
// Saves on exit,
// and delegates update/draw as well.
void Engine::run() {
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = "polyedit_gui_config.ini";
	while (window->isOpen()) {
		sf::Event event;
		while (window->pollEvent(event)) {
			if (event.type == sf::Event::Closed) {
				saveJSON();
				ImGui::SFML::Shutdown();
				window->close();
				std::exit(1);
			}
			// Handle events in relation to the GUI
			handleGUItoggleEvent(event);
			// If the GUI is open pass events to it and block left clicks for all GUIS
			if (showColorPickerGUI || showSettingsGUI) {
				ImGui::SFML::ProcessEvent(event);
				if (!(event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left)){
					handleEvents(event);
				}
			}
			// If the GUI is closed, run all events regardless
			else {
				handleEvents(event);
			}
		}
		window->clear(BGColor);
		// If a GUI is up update them
		if (showColorPickerGUI || showSettingsGUI) {
			ImGui::SFML::UpdateImGui();
			ImGui::SFML::UpdateImGuiRendering();

			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8,8));
			
			ImGuiStyle& style = ImGui::GetStyle();
			style.Colors[ImGuiCol_TitleBg] = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
			style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
			style.Colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
			style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 0.94f, 0.98f, 0.32f);
			style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.80f);

			ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(105, 123, 151, 255));
			if (showColorPickerGUI){
				createColorPickerGUI();
			}
			if (showSettingsGUI){
				createSettingsGUI();
				//ImGui::ShowStyleEditor();
			}
		}
		// Main loop
		update();
		draw();
		// Render UI
		if (showColorPickerGUI || showSettingsGUI) {
			ImGui::Render();
		}
		window->display();
	}
}

// Loads an image into the engine variables,
// storing the names of the files to save into vfile and sfile.
// If the files do not exist, they are created.
int Engine::load(){
	const char* filter[3] = { "*.png", "*.jpg", "*.gif" };
    const char* filenamecc = tinyfd_openFileDialog("Select image: ", "./", 3, filter, NULL, 0);
	if (filenamecc == NULL){
		return 1;
	}
	std::string filename = filenamecc;
	// Strip extension
	size_t lastindex = filename.find_last_of(".");
	std::string filenoext = filename.substr(0, lastindex);
	// Add new extensions
	const std::string sfext = ".svg";
	const std::string vfext = ".vertices";
	vfile = filenoext + vfext;
	sfile = filenoext + sfext;
	// Open/create based on if the file exists
	std::fstream vstream;
	vstream.open(vfile, std::ios::in);
	if (!vstream){
		vstream.open(vfile, std::ios::out);
	}
	std::fstream sstream;
	sstream.open(sfile, std::ios::in);
	if (!sstream){
		sstream.open(sfile, std::ios::out);
	}
	// Fail if the images do not open correctly
	if (!(image.loadFromFile(filename))){
		return 1;
	}
	if (!(img.loadFromFile(filename))){
		return 1;
	}
	// Load json containing points, etc.
	loadJSON();
	vstream.close();
	sstream.close();
	return 0;
}

// Check if the GUI is being toggled; pause other inputs if it is
void Engine::handleGUItoggleEvent(sf::Event event) {
	if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::C) {
		showColorPickerGUI = !showColorPickerGUI;
	}
	if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
		showSettingsGUI = !showSettingsGUI;
	}
}

// Handles the events that the sfml window recieves.
// The following events are handled:
//     Single press events (and their releases)
//     Click events (and their releases)
//     Scrolling
//     Resizing
void Engine::handleEvents(sf::Event event){
	// On single press event
	if (event.type == sf::Event::KeyPressed) {
		std::string text;
        // Toggles image smoothing
		if (event.key.code == sf::Keyboard::Slash){
			smoothnessToggle();
			imgsmooth ? text = "on" : text = "off";
			std::cout << "Image smoothing " << text << " (Slash)\n";
		}
        // Wireframe toggle for polygons
		if (event.key.code == sf::Keyboard::W){
			wireframe = !wireframe;
			wireframe ? text = "on" : text = "off";
			std::cout << "Wireframe " << text << " (W)\n";
		}
        // Hides background
		if (event.key.code == sf::Keyboard::H){
			hideimage = !hideimage;
			hideimage ? text = "hidden" : text = "shown";
			std::cout << "Background " << text << " (H)\n";
		}
        if (event.key.code == sf::Keyboard::X){
            showcenters = !showcenters;
            showcenters ? text = "Showing" : text = "Hiding";
            std::cout << text << " centers. (X)\n";
        }
		if (event.key.code == sf::Keyboard::P){
			showrvectors = !showrvectors;
			showrvectors ? text = "Showing" : text = "Hiding";
			std::cout << text << " points. (P)\n";
		}
        // Clears current selection
		if (event.key.code == sf::Keyboard::Space){
			clearSelection();
            std::cout << "Clearing selection (Spacebar) \n";
		}
        // Saves the file as a set of a SVG and ".vertices" file
		if (event.key.code == sf::Keyboard::S){
			saveVector(file);
			saveJSON();
            std::cout << "Saving file (S) \n";
		}
        // Camera panning without mousewheelclick
		if (event.key.code == sf::Keyboard::LControl){
            std::cout << "Panning while button held (LControl)\n";
			sf::Vector2i pointi = sf::Mouse::getPosition(*window);
			sf::Vector2f point;
			point.x = (float)pointi.x;
			point.y = (float)pointi.y;
			point = windowToGlobalPos(point);
			vdraginitpt = point;
			vdragflag = true;
		}
        // Deletes selected points, polys
		if (event.key.code == sf::Keyboard::Delete){
            std::cout << "Deleting selection (Delete) \n";
			deleteSelection();
		}
        // Reaverage colors
		if (event.key.code == sf::Keyboard::A) {
            std::cout << "Re-averaging color in polygon (A) \n";
            if (spoly != NULL){
				pR polyRecolor;
				polyRecolor.polyColor = spoly->fillcolor;
				for (int i = 0; i < polygons.size(); ++i) {
					if (polygons[i].center == spoly->center) {
						polyRecolor.polyIndex = i;
					}
				}
				undoBuffer.push_back(UndoAction(polyRecolor));
                spoly->fillcolor = sf::Color(avgClr(rpoints[spoly->rpointIndices[0]], rpoints[spoly->rpointIndices[1]], rpoints[spoly->rpointIndices[2]], 10));
            } else {
                "Can't change color - no polygon selected (C) \n";
            }
        }
        // Get color at mouse
		if (event.key.code == sf::Keyboard::O) {
            std::cout << "Setting selected polygon color to color at mouse (O)\n";
            if (spoly != NULL){
                sf::Vector2f point = getMPosFloat();
                point = windowToGlobalPos(point);
				point = getClampedImgPoint(point);
                sf::Color color = img.getPixel(point.x, point.y);

				pR polyRecolor;
				polyRecolor.polyColor = spoly->fillcolor;
				for (int i = 0; i < polygons.size(); ++i) {
					if (polygons[i].center == spoly->center) {
						polyRecolor.polyIndex = i;
					}
				}
				undoBuffer.push_back(UndoAction(polyRecolor));

                spoly->fillcolor = color;
            } else {
                "Can't change color - no polygon selected (C) \n";
            }
        }
		// Send overlapping element to end of list
		if (event.key.code == sf::Keyboard::Comma){
			std::cout << "Sending triangle to the back of the draw order (,)\n";
			if (spoly != NULL){
				for (unsigned i = 0; i < polygons.size(); i++){
					if (polygons[i].selected == true){
						polygons.insert(polygons.begin(), polygons[i]);
						polygons.erase(1 + polygons.begin() + i);
					}
				}
				clearSelection();
			}
		}
		// Send overlapping element to the beginning of the list
		if (event.key.code == sf::Keyboard::Period){
			std::cout << "Sending triangle to the front of the draw order (.)\n";
			if (spoly != NULL){
				for (unsigned i = 0; i < polygons.size(); i++){
					if (polygons[i].selected == true){
						polygons.push_back(polygons[i]);
						polygons.erase(polygons.begin() + i);
					}
				}
				clearSelection();
			}
		}

		//dbgPrint info
		if (event.key.code == sf::Keyboard::BackSlash) {
			std::cout << "=== Undo Buffer Dump ===" << std::endl;
			std::cout << "=== Actions: " << undoBuffer.size() << " ===" << std::endl;
			int index = 0;
			for (auto e : undoBuffer) {
				std::cout << "#" << index << " ";
				e.print();
				++index;
			}
		}
		if (event.key.code == sf::Keyboard::Z) {
			std::cout << "Undo\n";
			undo();
		}
		if (event.key.code == sf::Keyboard::RBracket) {
			undoBuffer.clear();
		}
	}

	// On release event (used for disabling flags)
	if (event.type == sf::Event::KeyReleased){
		if (event.key.code == sf::Keyboard::LControl){
			vdragflag = false;
		}
	}

	// On click
	if (event.type == sf::Event::MouseButtonPressed) {
		sf::Vector2f click = sf::Vector2f((float)event.mouseButton.x, (float)event.mouseButton.y);
		sf::Vector2f point = windowToGlobalPos(click);
		if (event.mouseButton.button == sf::Mouse::Left) {
			onLeftClick(point);
		}
		if (event.mouseButton.button == sf::Mouse::Right){
			onRightClick(point);
		}
		if (event.mouseButton.button == sf::Mouse::Middle){
			onMiddleClick(point);
		}
	}

	// On click release (used for disabling flags)
	if (event.type == sf::Event::MouseButtonReleased){
		if (event.mouseButton.button == sf::Mouse::Left){
			dragflag = false;
		}
		if (event.mouseButton.button == sf::Mouse::Middle){
			vdragflag = false;
		}
	}

	// On scroll (used for zooming)
	if (event.type == sf::Event::MouseWheelScrolled){
		if (event.mouseWheelScroll.delta < 0){
			viewzoom *= 1.5;
			view.zoom(1.5);
			window->setView(view);
		}
		else if (event.mouseWheelScroll.delta > 0){
			viewzoom *= 0.75;
			view.zoom(0.75);
			window->setView(view);
		}
	}

	// On window resize
	if (event.type == sf::Event::Resized){
		view.reset(sf::FloatRect(0, 0, (float)event.size.width, (float)event.size.height));
		viewzoom = 1;
		window->setView(view);
	}
}

// Handles logic directly before drawing.
// This function runs every frame.
void Engine::update(){
	handleCamera();

	if (wireframe == true && (wireframe != wireframels)){
		for (Poly& polygon : polygons){
			polygon.isWireframe = true;
		}
		wireframels = wireframe;
	}
	else if (wireframe != true && (wireframe != wireframels)){
		for (Poly& polygon : polygons){
			polygon.isWireframe = false;
		}
		wireframels = wireframe;
	}
	if (dragflag){
		sf::Vector2f point = getMPosFloat();
		point = windowToGlobalPos(point);
		sf::Vector2f rpoint = rpoints[nindex].vector;
		rpoints[nindex].vector = (pdragoffset + point);
	}
	if (vdragflag){
		sf::Vector2f point = getMPosFloat();
		point = windowToGlobalPos(point);
		vdragoffset.x = (vdraginitpt.x - point.x);
		vdragoffset.y = (vdraginitpt.y - point.y);
		view.move(vdragoffset);
		window->setView(view);
	}
	for (Poly& polygon : polygons) {
		polygon.updateCShape(viewzoom);
		polygon.updateCenter();
	}
	for (Point& point : rpoints) {
		point.colorSelected = pointSelectedColor;
		point.colorIdle = pointIdleColor;
		point.updateCShape(viewzoom);
		point.vector = getClampedImgPoint(point.vector);
	}
}

// Draws the objects to the screen.
// This function runs every frame, preceded by Engine::update().
void Engine::draw(){
	// Reset view incase other objects change it
	window->setView(view);
	if (!hideimage){
		window->draw(drawimg);
	}
	for (Poly polygon : polygons){
		if (polygon.selected == false){
			window->draw(polygon.cshape);
		}
	}
	if (showrvectors){
		for (Point& point : rpoints){
			window->draw(point.cshape);
		}
	}
	for (Poly polygon : polygons){
		if (polygon.selected == true){
			window->draw(polygon.cshape);
		}
	}
    if (showcenters){
        for (Poly polygon : polygons){
            sf::CircleShape cshape = sf::CircleShape(2*viewzoom);
            cshape.setPosition(polygon.center);
			cshape.setFillColor(centerdrawcolor);
            window->draw(cshape);
        }
    }
}

// Create GUI elements for color picker
void Engine::createColorPickerGUI() {
	ImGui::Begin("Color Picker");
	if (spoly != NULL){
		float spolycolor[3];
		spolycolor[0] = spoly->fillcolor.r / 255.0f;
		spolycolor[1] = spoly->fillcolor.g / 255.0f;
		spolycolor[2] = spoly->fillcolor.b / 255.0f;
		if (ColorPicker3(spolycolor)){
			spoly->fillcolor = sf::Color(spolycolor[0] * 255.0f, spolycolor[1] * 255.0f, spolycolor[2] * 255.0f, 255);
		}
	}
	else {
		ImGui::Text("No polygon selected.");
	}
	ImGui::End();
}

void Engine::createSettingsGUI(){
	ImGui::Begin("Settings");
	if (ImGui::CollapsingHeader("Points: Unselected")){
		float colorIdle[3];
		colorIdle[0] = rpoints[0].colorIdle.r / 255.0f;
		colorIdle[1] = rpoints[0].colorIdle.g / 255.0f;
		colorIdle[2] = rpoints[0].colorIdle.b / 255.0f;
		if (ColorPicker3(colorIdle)){
			pointIdleColor = sf::Color(colorIdle[0] * 255.0f, colorIdle[1] * 255.0f, colorIdle[2] * 255.0f, 255);
		}
	}
	if (ImGui::CollapsingHeader("Points: Selected")){
		float colorSelected[3];
		colorSelected[0] = rpoints[0].colorSelected.r / 255.0f;
		colorSelected[1] = rpoints[0].colorSelected.g / 255.0f;
		colorSelected[2] = rpoints[0].colorSelected.b / 255.0f;
		if (ColorPicker3(colorSelected)){
			pointSelectedColor = sf::Color(colorSelected[0] * 255.0f, colorSelected[1] * 255.0f, colorSelected[2] * 255.0f, 255);
			/*for (Point& p : rpoints){
				p.colorSelected = sf::Color(colorSelected[0] * 255.0f, colorSelected[1] * 255.0f, colorSelected[2] * 255.0f, 255);
			}*/
		}
	}
	if (ImGui::CollapsingHeader("Centers")){
		float center[4];
		center[0] = centerdrawcolor.r / 255.0f;
		center[1] = centerdrawcolor.g / 255.0f;
		center[2] = centerdrawcolor.b / 255.0f;
		center[3] = centerdrawcolor.a / 255.0f;
		if (ColorPicker4(center, true)){
			centerdrawcolor = sf::Color(center[0]*255.0f, center[1]*255.0f, center[2]*255.0f, center[3]*255.0f);
		}
	}
	if (ImGui::CollapsingHeader("Background Color")) {
		float bg[3];
		bg[0] = BGColor.r / 255.0f;
		bg[1] = BGColor.g / 255.0f;
		bg[2] = BGColor.b / 255.0f;
		if (ColorPicker3(bg)) {
			BGColor = sf::Color(bg[0] * 255.0f, bg[1] * 255.0f, bg[2] * 255.0f,255.0f);
		}
	}
	ImGui::End();
}

// Checks input for arrow keys and +/- for camera movement
void Engine::handleCamera() {
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left) && window->hasFocus()) {
		view.move(sf::Vector2f(-2 * viewzoom, 0));
		window->setView(view);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right) && window->hasFocus()) {
		view.move(sf::Vector2f(2 * viewzoom, 0));
		window->setView(view);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up) && window->hasFocus()) {
		view.move(sf::Vector2f(0, -2 * viewzoom));
		window->setView(view);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down) && window->hasFocus()) {
		view.move(sf::Vector2f(0, 2 * viewzoom));
		window->setView(view);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Dash) && window->hasFocus()) {
		viewzoom *= 1.01f;
		view.zoom(1.01f);
		window->setView(view);
	}
	if (sf::Keyboard::isKeyPressed(sf::Keyboard::Equal) && window->hasFocus()) {
		viewzoom *= 0.99f;
		view.zoom(0.99f);
		window->setView(view);
	}
}

/*////////////////////////////////////////////////////////////////////////////
//// Input functions:
//// Callbacks from Engine::handleEvents().
*/////////////////////////////////////////////////////////////////////////////

// On spacebar
void Engine::clearSelection() {
	nspoints.clear();
	spointsin.clear();
	spoly = NULL;
	for (unsigned n = 0; n < rpoints.size(); n++) {
		nspoints.push_back(&(rpoints[n]));
		rpoints[n].selected = false;
	}
	for (unsigned i = 0; i < polygons.size(); i++) {
		polygons[i].selected = false;
	}
}

// On slash
void Engine::smoothnessToggle() {
	image.setSmooth(!imgsmooth);
	imgsmooth = !imgsmooth;
	drawimg.setTexture(image);
}

// On delete
void Engine::deleteSelection() {
	if (spointsin.size() == 0 && spoly == NULL) {}
	else {
		std::vector<int> polyIndices;
		std::vector<int> rpointsIndices;
		for (unsigned i = 0; i < polygons.size(); i++) {
			if (polygons[i].selected == true) {
				polyIndices.push_back(i);
			}
		}
		for (unsigned i = 0; i < spointsin.size(); i++) {
			for (unsigned j = 0; j < polygons.size(); j++) {
				for (unsigned k = 0; k < 3; k++) {
					if (polygons[j].rpointIndices[k] == spointsin[i]) {
						polyIndices.push_back(j);
					}
				}
			}
			rpointsIndices.push_back(spointsin[i]);
		}

		// Sort vectors and erase duplicates
		std::sort(polyIndices.begin(), polyIndices.end());
		polyIndices.erase(std::unique(polyIndices.begin(), polyIndices.end()), polyIndices.end());
		std::sort(rpointsIndices.begin(), rpointsIndices.end());
		rpointsIndices.erase(std::unique(rpointsIndices.begin(), rpointsIndices.end()), rpointsIndices.end());
		// Reverse indices for easier deletion of elements
		std::reverse(rpointsIndices.begin(), rpointsIndices.end());
		std::reverse(polyIndices.begin(), polyIndices.end());

		// Get polys to re-add in deletion of a point
		if (spointsin.size() > 0 && spoly == NULL) { // get deleted points
			PD pointDeletion = PD();
			for (int i : spointsin) {
				pointDeletion.deletedPoints.push_back(std::make_pair(rpoints[i],i));
			}
			for (int i : polyIndices) {
				pointDeletion.cPointColors.push_back(polygons[i].fillcolor);
				auto arrayLoc = polygons[i].rpointIndices;
				int p1 = arrayLoc[0];
				int p2 = arrayLoc[1];
				int p3 = arrayLoc[2];
				std::cout << p1 << p2 << p3 << "\n";
				pointDeletion.cPointPolysIndices.push_back(std::make_tuple(p1, p2, p3));
			}
			undoBuffer.push_back(UndoAction(pointDeletion));
		}

		for (unsigned i = 0; i < polyIndices.size(); i++) {
			polygons.erase(polygons.begin() + polyIndices[i]);
		}
		for (unsigned i = 0; i < rpointsIndices.size(); i++) {
			rpoints.erase(rpoints.begin() + rpointsIndices[i]);
		}
		for (unsigned p = 0; p < polygons.size(); p++) { // For each polygon
			for (int i = 0; i < 3; i++) { // For every point inside a polygon
				int counter = 0;
				for (unsigned rp = 0; rp < rpointsIndices.size(); rp++) {
					if (polygons[p].rpointIndices[i] > rpointsIndices[rp]) {
						counter++;
					}
				}
				polygons[p].rpointIndices[i] -= counter;
			}
		}

		polyIndices.clear();
		rpointsIndices.clear();
		clearSelection();
		for (Poly& poly : polygons){
			poly.updatePointers(rpoints);
		}
	}
}

// On left click
void Engine::onLeftClick(sf::Vector2f point) {
	for (Poly& polygon : polygons) {
		polygon.selected = false;
	}
	bool ispointnear = false;
	nindex = -1;
	for (unsigned i = 0; i < rpoints.size(); i++) {
		Point& rpoint = rpoints[i];
		sf::Vector2f rpv = rpoint.vector;
		int clickdist = GRABDIST;
        // Check if mouse is in snapping range; then set near appropriately
		if ((point.x - rpv.x < clickdist*viewzoom && point.x - rpv.x > -clickdist*viewzoom) &&
			(point.y - rpv.y < clickdist*viewzoom && point.y - rpv.y > -clickdist*viewzoom)) {
			ispointnear = true;
			nindex = i;
		}
	}
	// If its near another, snap to it -> shared edges
	if (ispointnear) {
		sf::Vector2f mpos = getMPosFloat();
		mpos = windowToGlobalPos(mpos);
		// Init values for dragging, used above
		pdraginitpt = mpos;
		pdragoffset.x = rpoints[nindex].vector.x - mpos.x;
		pdragoffset.y = rpoints[nindex].vector.y - mpos.y;
		dragflag = true; // When dragflag is true then dragging occurs
						 // Set mouse position to middle of desired selected point
						 // This fixes mouse clicks moving points on accident
		// Check for snapping the same point twice for a new poly and catch it
		bool exists = false;
		for (unsigned i = 0; i < spointsin.size(); i++) {
			if (spointsin[i] == nindex) {
				exists = true;
			}
		}
		// If they didn't click the same point twice
		if (!exists) {
			spointsin.push_back(nindex);
			if (spointsin.size() == 3) {
				int p1, p2, p3;
				p1 = spointsin[spointsin.size() - 3];
				p2 = spointsin[spointsin.size() - 2];
				p3 = spointsin[spointsin.size() - 1];
				Point *po1 = &rpoints[(spointsin[0])];
				Point *po2 = &rpoints[(spointsin[1])];
				Point *po3 = &rpoints[(spointsin[2])];
				Poly pg = Poly(po1, po2, po3, p1, p2, p3, sf::Color::Green);
				polygons.push_back(pg);
				int offset = polygons.size() - 1;
				polygons[offset].fillcolor = avgClr(rpoints[polygons[offset].rpointIndices[0]], rpoints[polygons[offset].rpointIndices[1]], rpoints[polygons[offset].rpointIndices[2]], 10);
				
				// Add undo
				pA polyAddition = pA();
				polyAddition.polyIndex = offset;
				undoBuffer.push_back(UndoAction(polyAddition));
				
				clearSelection();
			}
		}
	}
	// Create a new point
	if (!ispointnear) {
		point = getClampedImgPoint(point);
		rpoints.push_back(Point(point, 5));
		
		// Add undo
		PA pointAddUndoAction = PA();
		pointAddUndoAction.pointIndex = rpoints.size()-1;
		undoBuffer.push_back(UndoAction(pointAddUndoAction));

		spointsin.push_back(rpoints.size() - 1);
		if (spointsin.size() == 3) {
			int p1, p2, p3;
			p1 = spointsin[spointsin.size() - 3];
			p2 = spointsin[spointsin.size() - 2];
			p3 = spointsin[spointsin.size() - 1];
			Point *po1 = &rpoints[(spointsin[0])];
			Point *po2 = &rpoints[(spointsin[1])];
			Point *po3 = &rpoints[(spointsin[2])];
			// Create new polygon at last 3 points, assign it starting index
			// so that it can find itself in the vector after memory updates
			polygons.push_back(Poly(po1,po2,po3,p1,p2,p3,sf::Color::Green));
			int offset = polygons.size() - 1;
			polygons[offset].fillcolor = avgClr(rpoints[polygons[offset].rpointIndices[0]], rpoints[polygons[offset].rpointIndices[1]], rpoints[polygons[offset].rpointIndices[2]], 10);
			
			// Add undo
			pA polyAddition = pA();
			polyAddition.polyIndex = offset;
			undoBuffer.push_back(UndoAction(polyAddition));

			clearSelection();
		}
	}
	// Only update polygon point-pointers on click
	// because the memory is only moved on click
	for (Poly& p : polygons) {
		p.updatePointers(rpoints);
	}
	for (auto& inspoint : nspoints) {
		inspoint->selected = false;
	}
	for (auto& ispoint : spointsin) {
		rpoints[ispoint].selected = true;
	}
}

// On right click
void Engine::onRightClick(sf::Vector2f point) {
	if (polygons.size() > 0) {
		clearSelection();
		float dist = 100000000.0f;
		int pindex = -1;
		for (unsigned p = 0; p < polygons.size(); p++) {
			float cdist = v2fdistance(point, polygons[p].center);
			if (cdist < dist) {
				dist = cdist;
				pindex = p;
			}
		}
		polygons[pindex].selected = true;
		spoly = &polygons[pindex];
	}
}

// On middle wheel click
void Engine::onMiddleClick(sf::Vector2f point) {
	vdragflag = true;
	vdragoffset = sf::Vector2f(0, 0);
	vdraginitpt = point;
}

void Engine::undo() {
	if (undoBuffer.size() > 0) {
		UndoAction ua = undoBuffer.back();
		undoBuffer.pop_back();

		PA dPA;
		PD dPD;
		pD dpD;
		pA dpA;
		pR dpR;

		switch (ua.action) {
		// Simply delete the last added point
		// Not exactly simple, as the delete function only runs off the current selection
		// Hacky workaround for just deleting a point: select it, delete selection, clear
		case Action::pointAddition:
			dPA = ua.pointAddition;
			clearSelection();
			spoint = &rpoints[dPA.pointIndex];
			spointsin.push_back(dPA.pointIndex);
			deleteSelection();
			// Manually deselect the last two points and pop the deletion action
			undoBuffer.pop_back();
			(&rpoints[dPA.pointIndex - 2])->selected = false;
			(&rpoints[dPA.pointIndex - 3])->selected = false;
			break;
		case Action::pointDeletion:
			// there's either going to be one or two points deleted
			dPD = ua.pointDeletion;
			clearSelection();
			// Re-add the deleted points
			for (auto& p : dPD.deletedPoints) {
				rpoints.push_back(p.first);
				std::cout << "Added point " << (rpoints.size()-1) << ";\n";
				nspoints.push_back(&p.first);
			}
			// Reconstruct polygons that were caught in the point deletion
			for (int index = 0; index < dPD.cPointPolysIndices.size(); ++index) {
				// is dp2 active (were 2 points deleted)
				bool dp2a = true;
				// Polygon color
				sf::Color pcolor = dPD.cPointColors[index];
				// Indices of old deleted points
				int dp1, dp2;
				// Indices of the whole polygon deleted
				int i1, i2, i3;
				// Pointers to points of the new polygon
				Point *p1, *p2, *p3;
				// Offset to end of rpoints
				int offset = rpoints.size() - 1;

				// Get destroyed polygon indices (its in a tuple<int,int,int>)
				i1 = std::get<0>(dPD.cPointPolysIndices[index]);
				i2 = std::get<1>(dPD.cPointPolysIndices[index]);
				i3 = std::get<2>(dPD.cPointPolysIndices[index]);

				// Find if one or two points were deleted, set dp2a on results
				dp1 = dPD.deletedPoints[0].second;
				(dPD.deletedPoints.size() > 1) ? dp2 = dPD.deletedPoints[1].second : dp2a = false;

				// Find which point in the deleted-by-consequence polygon was the deleted point
				// and update a new polygon replacing the deleted point with the newly added point
				int oi1, oi2, oi3;
				oi1 = i1;
				oi2 = i2;
				oi3 = i3;
				if (i1 > dp1) i1 -= 1;
				if (i2 > dp1) i2 -= 1;
				if (i3 > dp1) i3 -= 1;
				if (dp2a) {
					if (oi1 > dp2) i1 -= 1;
					if (oi2 > dp2) i2 -= 1;
					if (oi3 > dp2) i3 -= 1;
				}
				p1 = &rpoints[i1];
				p2 = &rpoints[i2];
				p3 = &rpoints[i3];
				if (!dp2a) {
					if (oi1 == dp1) {
						p1 = &rpoints[offset];
						i1 = offset;
					}
					else if (oi2 == dp1) {
						p2 = &rpoints[offset];
						i2 = offset;
					}
					else if (oi3 == dp1) {
						p3 = &rpoints[offset];
						i3 = offset;
					}
				}
				if (dp2a) {
					if (oi1 == dp1) {
						p1 = &rpoints[offset - 1];
						i1 = offset - 1;
					}
					else if (oi2 == dp1) {
						p2 = &rpoints[offset - 1];
						i2 = offset - 1;
					}
					else if (oi3 == dp1) {
						p3 = &rpoints[offset - 1];
						i3 = offset -1;
					}
					if (oi1 == dp2) {
						p1 = &rpoints[offset];
						i1 = offset;
					}
					else if (oi2 == dp2) {
						p2 = &rpoints[offset];
						i2 = offset;
					}
					else if (oi3 == dp2) {
						p3 = &rpoints[offset];
						i3 = offset;
					}
				}
				std::cout << "New Points: " << i1 << i2 << i3 << '\n';
				polygons.push_back(Poly(p1, p2, p3, i1, i2, i3, pcolor));
			}
			break;
		// Select the polygon -> delete selection -> clear.
		case Action::polyAddition:
			dpA = ua.polyAddition;
			clearSelection();
			spoly = &polygons[dpA.polyIndex];
			(&polygons[dpA.polyIndex])->selected = true;
			deleteSelection();
			break;
		case Action::polyRecolor:
			dpR = ua.polyRecolor;
			polygons[dpR.polyIndex].fillcolor = dpR.polyColor;
			break;
		case Action::polyDeletion:
			dpD = ua.polyDeletion;
			Point *p1, *p2, *p3;
			
			int i1 = dpD.pointIndices[0];
			int i2 = dpD.pointIndices[1];
			int i3 = dpD.pointIndices[2];
			p1 = &(rpoints[i1]);
			p2 = &(rpoints[i2]);
			p3 = &(rpoints[i3]);
			polygons.push_back(Poly(p1, p2, p3, i1, i2, i3, dpD.polyColor));
			break;
		}

		clearSelection();
	}
}

/*/////////////////////////////////////////////////////////////////////////////
//// Utility functions
*//////////////////////////////////////////////////////////////////////////////

// Returns the average color from the area between 3 points.
// Due to speed, it uses random samples.
sf::Color Engine::avgClr(Point& p1, Point& p2, Point& p3, int samples){
	int r = 0;
	int g = 0;
	int b = 0;
	for (int i = 0; i < samples; i++){
		sf::Vector2f pixel = randPt(p1, p2, p3);
		sf::Color color = img.getPixel(pixel.x, pixel.y);
		r += color.r;
		g += color.g;
		b += color.b;
	}
	r /= samples;
	g /= samples;
	b /= samples;
	return sf::Color(r, g, b, 255);
}

// Return a random point inside 3 points.
sf::Vector2f Engine::randPt(Point& p1, Point& p2, Point& p3){
	sf::Vector2f point;
	double r1 = randdbl(0, 1);
	double r2 = randdbl(0, 1);
	point.x = (1 - sqrt(r1)) * p1.vector.x + (sqrt(r1) * (1 - r2)) * p2.vector.x + (sqrt(r1) * r2) * p3.vector.x;
	point.y = (1 - sqrt(r1)) * p1.vector.y + (sqrt(r1) * (1 - r2)) * p2.vector.y + (sqrt(r1) * r2) * p3.vector.y;
	return point;
}

// Convert window (view) coordinates to global (real) coordinates.
sf::Vector2f Engine::windowToGlobalPos(const sf::Vector2f& vec) {
	sf::Vector2u winSize = window->getSize();
	sf::Vector2f center = view.getCenter();
	sf::Vector2f point = vec;
	point.x -= winSize.x / 2;
	point.y -= winSize.y / 2;
	point.x *= viewzoom;
	point.y *= viewzoom;
	point.x += center.x;
	point.y += center.y;
	return point;
};

// Convert global (real) coordiantes to window (view) coordinates.
sf::Vector2f Engine::globalToWindowPos(const sf::Vector2f& vec) {
	sf::Vector2u winSize = window->getSize();
	sf::Vector2f center = view.getCenter();
	sf::Vector2f point = vec;
	point.x -= center.x;
	point.y -= center.y;
	point.x /= viewzoom;
	point.y /= viewzoom;
	point.x += winSize.x / 2;
	point.y += winSize.y / 2;
	return point;
};

// Get the mouse position as a float.
sf::Vector2f Engine::getMPosFloat() {
	sf::Vector2i mposi = sf::Mouse::getPosition(*window);
	sf::Vector2f mpos;
	mpos.x = mposi.x;
	mpos.y = mposi.y;
	return mpos;
}

// Clamps a point to the image boundaries.
sf::Vector2f Engine::getClampedImgPoint(const sf::Vector2f& vec){
	sf::Vector2f result = vec;
	if (result.x < 0 || result.x > img.getSize().x){
		if (result.x < 0){
			result.x = 0;
		}
		else if (result.x > img.getSize().x) {
			result.x = img.getSize().x;
		}
	}
	if (result.y < 0 || result.y > img.getSize().y){
		if (result.y < 0){
			result.y = 0;
		}
		else if (result.y > img.getSize().y){
			result.y = img.getSize().y;
		}
	}
	return result;
}


/*////////////////////////////////////////////////////////////////////////////
//// Saving functions
*/////////////////////////////////////////////////////////////////////////////

// Saves the SVG of the image.
void Engine::saveVector(std::string filename){
	std::fstream sfilestrm;
	sfilestrm.open(sfile, std::ios::out | std::fstream::trunc);
	char headerc[350];
	const char *hdr = "<?xml version=\"1.0\" standalone=\"no\"?>\n<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\"><svg width=\"%d\" height=\"%d\" viewBox=\"0 0 %d %d\" xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">\n<style type=\"text/css\"> polygon { stroke-width: .5; stroke-linejoin: round; } </style>";
	snprintf(headerc, sizeof(headerc), hdr,
		image.getSize().x,
		image.getSize().y,
		image.getSize().x,
		image.getSize().y);
	std::string header = headerc;
	std::string footer = "\n</svg>";
	sfilestrm << header;
	for (unsigned i = 0; i < polygons.size(); i++){
		Poly& p = polygons[i];
		std::string pointslist = "";
		for (int j = 0; j < 3; j++){
			pointslist += std::to_string(rpoints[p.rpointIndices[j]].vector.x);
			pointslist += ",";
			pointslist += std::to_string(rpoints[p.rpointIndices[j]].vector.y);
			pointslist += " ";
		}
		std::string color = "rgb(";
		color += std::to_string(p.fillcolor.r);
		color += ",";
		color += std::to_string(p.fillcolor.g);
		color += ",";
		color += std::to_string(p.fillcolor.b);
		color += ")";
		std::string polygon = "";
		polygon += "<polygon style=\"fill:";
		polygon += color;
		polygon += ";stroke:";
		polygon += color;
		polygon += "\"";
		polygon += " points=\"";
		polygon += pointslist;
		polygon += "\"/>\n";
		sfilestrm << polygon;
	}
	sfilestrm << footer;
}

// Saves the JSON of the points, polygons, colors
void Engine::saveJSON(){
	// Clamp all points to bounds
	for (Point& point : rpoints){
		point.vector = getClampedImgPoint(point.vector);
	}
	Json::Value rootobj;
	for (unsigned i = 0; i < rpoints.size(); i++){
		rootobj["rpoints"][i]["vector"]["x"] = rpoints[i].vector.x;
		rootobj["rpoints"][i]["vector"]["y"] = rpoints[i].vector.y;
		rootobj["rpoints"][i]["size"] = rpoints[i].size;
	}
	for (unsigned i = 0; i < polygons.size(); i++){
		for (int j = 0; j < 3; j++){
			rootobj["polygons"][i]["pointindices"][j] = polygons[i].rpointIndices[j];
		}
		rootobj["polygons"][i]["color"] = polygons[i].fillcolor.toInteger();
	}
	rootobj["centerdrawcolor"] = centerdrawcolor.toInteger();
	rootobj["bgcolor"] = BGColor.toInteger();
	rootobj["colorIdle"] = pointIdleColor.toInteger();
	rootobj["colorSelected"] = pointSelectedColor.toInteger();
	std::fstream vfilestrm;
	vfilestrm.open(vfile, std::ios::out | std::ios::trunc);
	vfilestrm << rootobj << std::endl;
	vfilestrm.close();
}

// Loads the JSON into the engine variables.
void Engine::loadJSON(){
	rpoints.clear();
	polygons.clear();
	std::fstream vfilestrm;
	vfilestrm.open(vfile, std::ios::in);
	Json::Value rootobj;
	if (vfilestrm.peek() == std::fstream::traits_type::eof()) {
		return;
	}
	vfilestrm >> rootobj;
	Json::Value jsonpolygons;
	Json::Value jsonrpoints;
	jsonpolygons = rootobj["polygons"];
	jsonrpoints = rootobj["rpoints"];
	for (unsigned i = 0; i < jsonrpoints.size(); i++){
		Point p;
		p.vector.x = rootobj["rpoints"][i]["vector"]["x"].asFloat();
		p.vector.y = rootobj["rpoints"][i]["vector"]["y"].asFloat();
		p.size = rootobj["rpoints"][i]["size"].asFloat();
		// Init the color to default before reading user-set because of backwards compatibility
		p.colorIdle = sf::Color::Green;
		p.colorSelected = sf::Color::Blue;
		centerdrawcolor = sf::Color(255, 255, 255, 127);
		// Read JSON for colors
		rpoints.push_back(p);
	}
	for (unsigned i = 0; i < jsonpolygons.size(); i++){
		int ptl[3];
		for (int j = 0; j < 3; j++){
			ptl[j] = rootobj["polygons"][i]["pointindices"][j].asInt();
		}
		int c = rootobj["polygons"][i]["color"].asInt64();
		sf::Color color = sf::Color(c);
		Poly p = Poly(&rpoints[ptl[0]],
					  &rpoints[ptl[1]],
					  &rpoints[ptl[2]], ptl[0], ptl[1], ptl[2], color);
		p.updateCenter();
		p.updateCShape(viewzoom);
		polygons.push_back(p);
	}
	if (rootobj["bgcolor"].asInt64() != 0) {
		BGColor = sf::Color(rootobj["bgcolor"].asInt64());
	}
	int color = rootobj["centerdrawcolor"].asInt64();
	if (color != 0) {
		centerdrawcolor = sf::Color(color);
	}
	pointIdleColor = sf::Color::Green;
	pointSelectedColor = sf::Color::Blue;
	color = rootobj["colorIdle"].asInt64();
	if (color != 0) {
		pointIdleColor = sf::Color(color);
	}
	color = rootobj["colorSelected"].asInt64();
	if (color != 0) {
		pointSelectedColor = sf::Color(color);
	}
	std	::cout << "total polygons loaded: " << polygons.size() << "\n";
	vfilestrm.close();
}
