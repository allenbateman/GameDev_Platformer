#include "App.h"
#include "Input.h"
#include "Textures.h"
#include "Audio.h"
#include "Render.h"
#include "Window.h"
#include "Scene1.h"
#include "Map.h"
#include "ModulePhysics.h"
#include "player.h"

#include "Defs.h"
#include "Log.h"

Scene1::Scene1(bool isActive) : Module(isActive)
{
	name.Create("scene");
}

// Destructor
Scene1::~Scene1()
{}

// Called before render is available
bool Scene1::Awake()
{
	LOG("Loading Scene 1");
	bool ret = true;

	return ret;
}

// Called before the first frame
bool Scene1::Start()
{
	// L03: DONE: Load map
	app->physics->Start();

	app->map->Load("level1.tmx");
	app->player->Spawn(iPoint(20, 300));

	// Load music
	//app->audio->PlayMusic("Assets/audio/music/music_spy.ogg");

	return true;
}

// Called each loop iteration
bool Scene1::PreUpdate()
{
	return true;
}

// Called each loop iteration
bool Scene1::Update(float dt)
{
    // L02: DONE 3: Request Load / Save when pressing L/S
	if(app->input->GetKey(SDL_SCANCODE_L) == KEY_DOWN)
		app->LoadGameRequest();

	if(app->input->GetKey(SDL_SCANCODE_S) == KEY_DOWN)
		app->SaveGameRequest();

	//app->render->DrawTexture(img, 380, 100); // Placeholder not needed any more

	// Draw map
	app->map->Draw();

	// L03: DONE 7: Set the window title with map/tileset info
	SString title("Map:%dx%d Tiles:%dx%d Tilesets:%d",
				   app->map->mapData.width, app->map->mapData.height,
				   app->map->mapData.tileWidth, app->map->mapData.tileHeight,
				   app->map->mapData.tilesets.count());

	app->win->SetTitle(title.GetString());

	return true;
}

// Called each loop iteration
bool Scene1::PostUpdate()
{
	bool ret = true;

	if(app->input->GetKey(SDL_SCANCODE_ESCAPE) == KEY_DOWN)
		ret = false;

	return ret;
}

// Called before quitting
bool Scene1::CleanUp()
{
	LOG("Freeing scene 1");
	app->player->Disable(); 
	app->map->CleanUp();
	app->physics->Disable();
	
	return true;
}

bool Scene1::LoadState(pugi::xml_node&)
{
	bool ret = true;
	return ret;
}

bool Scene1::SaveState(pugi::xml_node&) const
{
	bool ret = true;
	return ret;
}
void Scene1::OnCollision(PhysBody* bodyA, PhysBody* bodyB)
{
	b2Vec2 position;

	LOG("I got touched!");


	if (bodyA->type == Collider_Type::GEM || bodyB->type == Collider_Type::GEM)
	{
		LOG("GOT A GEM!");
	}

	if (bodyA->type == Collider_Type::DEATH || bodyB->type == Collider_Type::DEATH)
	{
		LOG("I DIED!");
	}
}