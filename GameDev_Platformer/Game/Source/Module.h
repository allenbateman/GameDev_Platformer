#ifndef __MODULE_H__
#define __MODULE_H__

#include "SString.h"
#include "PugiXml/src/pugixml.hpp"


class App;
class PhysBody;
class GuiControl;

class Module
{
public:

	Module(bool isActive) : active(isActive)
	{}

	void Init()
	{}

	// Called before render is available
	// L01: DONE 5: Sending config file to all modules
	virtual bool Awake(pugi::xml_node&)
	{
		return true;
	}

	// Called before the first frame
	virtual bool Start()
	{
		return true;
	}

	// Called each loop iteration
	virtual bool PreUpdate()
	{
		return true;
	}

	// Called each loop iteration
	virtual bool Update(float dt)
	{
		return true;
	}

	// Called each loop iteration
	virtual bool PostUpdate()
	{
		return true;
	}

	// Called before quitting
	virtual bool CleanUp()
	{
		return true;
	}

    // L02: DONE 2: Create new virtual methods to Load / Save state
	virtual bool LoadState(pugi::xml_node&)
	{
		return true;
	}

	virtual bool SaveState(pugi::xml_node&) const
	{
		return true;
	}

	virtual void OnCollision(PhysBody* bodyA, PhysBody* bodyB)
	{
	}

	virtual bool OnGuiMouseClickEvent(GuiControl* control)
	{
		return true;
	}

	void Module::Enable()
	{
		if (!active)
		{
			active = true;
			Start();
		}
	}

	void Module::Disable()
	{
		if (active)
		{
			active = false;
			CleanUp();
		}
	}

public:

	SString name;
	bool active;
	bool DEBUG = false;
};

#endif // __MODULE_H__