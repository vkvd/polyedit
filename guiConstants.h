#include "imgui/imgui.h"
#include "imgui/imconfig.h"
extern inline void applyGuiStyle(ImGuiStyle& style){
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.39f, 0.39f, 0.39f, 0.75f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.10f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.67f, 0.67f, 0.67f, 0.30f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.47f, 0.47f, 0.47f, 0.40f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.00f, 0.98f, 0.99f, 0.40f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.00f, 0.94f, 0.98f, 0.32f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.00f, 0.00f, 0.00f, 0.80f);

	style.ScrollbarRounding = 0.0f;
	style.ScrollbarSize = 20.0f;
	style.WindowRounding = 0.0f;
	style.WindowPadding = ImVec2(0.0f,0.0f);
}

extern inline void helpGuiText(){
	ImGui::Begin("Help");
	ImGui::Text("Mouse:\n-----------------");
	ImGui::Bullet(); ImGui::Text("Left click: Place or select point");
	ImGui::Bullet(); ImGui::Text("Right click: Select polygon by nearest center");
	ImGui::Bullet(); ImGui::Text("Middle click: Pan camera");
	ImGui::Bullet(); ImGui::Text("Scroll: Zoom");
	ImGui::NewLine();
	ImGui::Text("Keyboard:\n-----------------");
	ImGui::Bullet(); ImGui::Text("S: Save");
	ImGui::Bullet(); ImGui::Text("Z: Undo");
	ImGui::Bullet(); ImGui::Text("Delete: Delete selection");
	ImGui::Bullet(); ImGui::Text("Space: Clear selection");
	ImGui::Bullet(); ImGui::Text("Escape: Settings");

	ImGui::Bullet(); ImGui::Text("Toggles");
	ImGui::Indent();
	ImGui::Bullet(); ImGui::Text("H: Hide background image");
	ImGui::Bullet(); ImGui::Text("W: Wireframe mode");
	ImGui::Bullet(); ImGui::Text("X: Show polygon centers");
	ImGui::Bullet(); ImGui::Text("P: Hide points");
	ImGui::Bullet(); ImGui::Text("Forward slash: Toggle image smoothing when zoomed in");
	ImGui::Unindent();

	ImGui::Bullet(); ImGui::Text("Recolors");
	ImGui::Indent();
	ImGui::Bullet(); ImGui::Text("A: Re-average color");
	ImGui::Bullet(); ImGui::Text("C: Color Picker");
	ImGui::Bullet(); ImGui::Text("O: Set polygon color to color at mouse cursor");
	ImGui::Unindent();

	ImGui::Bullet(); ImGui::Text("Layers");
	ImGui::Indent();
	ImGui::Bullet(); ImGui::Text("Comma: Send to back");
	ImGui::Bullet(); ImGui::Text("Period: Send to front");
	ImGui::Unindent();

	ImGui::Bullet(); ImGui::Text("Trackpad helpers");
	ImGui::Indent(); 
	ImGui::Bullet(); ImGui::Text("+/-: Zoom");
	ImGui::Bullet(); ImGui::Text("Left Control + Mouse Drag: Pan camera");
	ImGui::Bullet(); ImGui::Text("Arrow Keys: Pan camera");
	ImGui::Unindent();

	ImGui::Bullet(); ImGui::Text("Debug (!)");
	ImGui::Indent();
	ImGui::Bullet(); ImGui::Text("Backslash: Dump undo buffer to console");
	ImGui::Bullet(); ImGui::Text("Right Bracket: Clear undo buffer");
	ImGui::Unindent();

	ImGui::End();
}