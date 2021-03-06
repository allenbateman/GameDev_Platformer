#ifndef __GUIMANAGER_H__
#define __GUIMANAGER_H__

#include "Module.h"

#include "GuiControl.h"

#include "List.h"

class GuiManager : public Module
{
public:

	// Constructor
	GuiManager(bool isActive);

	// Destructor
	virtual ~GuiManager();

	// Called before the first frame
	 bool Start();

	 bool Update(float dt);

	bool UpdateAll(float dt,bool logic);

	bool Draw();

	// Called before quitting
	bool CleanUp();

	//bool OnGuiMouseClickEvent(GuiControl* control);

	// Additional methods
	GuiControl* CreateGuiControl(GuiControlType type, int id, const char* text, int fontid, SDL_Rect bounds, Module* observer, SDL_Rect sliderBounds = { 0,0,0,0 });
	void DestroyGuiControl(GuiControl* entity);
	void AddGuiControl(GuiControl* entity);

public:

	List<GuiControl*> controls;

	float accumulatedTime = 0.0f;
	float updateMsCycle = 0.0f;
	bool doLogic = false;

	SDL_Texture* UItexture;

	int mainFont;
	int numberFont;
	bool Debug = false;


	
};

#endif // __GUIMANAGER_H__
