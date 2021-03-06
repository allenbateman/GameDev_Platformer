
#include "App.h"
#include "Render.h"
#include "Textures.h"
#include "Map.h"
#include "ModulePhysics.h"
#include "LevelManagement.h"
#include "ModuleEntities.h"
#include "Entity.h"
#include "Player.h"
#include "Scene1.h"
#include "Scene2.h"
#include<iostream>


#include "Defs.h"
#include "Log.h"
#include <math.h>

using namespace std;

Map::Map(bool isActive) : Module(isActive), mapLoaded(false)
{
    name.Create("map");
}

// Destructor
Map::~Map()
{}

int Properties::GetProperty(const char* value, int defaultValue) const
{
	ListItem<Property*>* item = list.start;

	while (item)
	{
		if (item->data->name == value)
			return item->data->value;
		item = item->next;
	}

	return defaultValue;
}

bool Properties::SetProperty(const char* name, int set_value) {


	ListItem<Property*>* item = list.start;

	while (item)
	{
		if (item->data->name == name)
		{
			item->data->value = set_value;
			return true;
		}
		item = item->next;
	}
	return false;
}

// Called before render is available
bool Map::Awake(pugi::xml_node& config)
{
    LOG("Loading Map Parser");
    bool ret = true;

    folder.Create(config.child("folder").child_value());

    return ret;
}

// Draw the map (all requried layers)
void Map::Draw()
{
	if (mapLoaded == false) return;

	ListItem<MapLayer*>* mapLayerItem;
	mapLayerItem = mapData.layers.start;

	while (mapLayerItem != NULL) {

		if (mapLayerItem->data->properties.GetProperty("Draw") == 1) {
			for (int x = 0; x < mapLayerItem->data->width; x++)
			{
				for (int y = 0; y < mapLayerItem->data->height; y++)
				{

					int gid = mapLayerItem->data->Get(x, y);

					if (gid > 0) {

						TileSet* tileset = GetTilesetFromTileId(gid);

						SDL_Rect r = tileset->GetTileRect(gid);
						iPoint pos = MapToWorld(x, y);

						app->render->DrawTexture(tileset->texture,pos.x, pos.y, &r);
					}
				}
			}
		}
		mapLayerItem = mapLayerItem->next;
	}	
}

//A method that translates x,y coordinates from map positions to world positions
iPoint Map::MapToWorld(int x, int y) const
{
	iPoint ret;

	//Isometric map to world coordinates
	if (mapData.type == MAPTYPE_ORTHOGONAL)
	{
		ret.x = x * mapData.tileWidth;
		ret.y = y * mapData.tileHeight;
	}
	else if (mapData.type == MAPTYPE_ISOMETRIC)
	{
		ret.x = (x - y) * (mapData.tileWidth / 2);
		ret.y = (x + y) * (mapData.tileHeight / 2);
	}
	else
	{
		LOG("Unknown map type");
		ret.x = x; ret.y = y;
	}

	return ret;
}

//Orthographic world to map coordinates
iPoint Map::WorldToMap(int x, int y) const
{
	iPoint ret(0, 0);

	//The case for isometric maps to WorldToMap
	if (mapData.type == MAPTYPE_ORTHOGONAL)
	{
		ret.x = x / mapData.tileWidth;
		ret.y = y / mapData.tileHeight;
	}
	else if (mapData.type == MAPTYPE_ISOMETRIC)
	{

		float half_width = mapData.tileWidth * 0.5f;
		float half_height = mapData.tileHeight * 0.5f;
		ret.x = int((x / half_width + y / half_height) / 2);
		ret.y = int((y / half_height - (x / half_width)) / 2);
	}
	else
	{
		LOG("Unknown map type");
		ret.x = x; ret.y = y;
	}

	return ret;
}

bool Map::CreateWalkabilityMap(int& width, int& height, uchar** buffer,int layerValue) const
{
	bool ret = false;
	ListItem<MapLayer*>* item;
	item = mapData.layers.start;

	for (item = mapData.layers.start; item != NULL; item = item->next)
	{
		MapLayer* layer = item->data;

		if (layer->properties.GetProperty("Navigation", 0) != layerValue)
			continue;
		
		uchar* map = new uchar[layer->width * layer->height];
		memset(map, 1, layer->width * layer->height);

		for (int y = 0; y < mapData.height; ++y)
		{
			for (int x = 0; x < mapData.width; ++x)
			{
				int i = (y * layer->width) + x;

				int tileId = layer->Get(x, y);
				TileSet* tileset = (tileId > 0) ? GetTilesetFromTileId(tileId) : NULL;

				if (tileset != NULL)
				{
					map[i] = (tileId - tileset->firstgid) > 0 ? 0 : 1;
				}
			}
		}

		*buffer = map;
		width = mapData.width;
		height = mapData.height;
		ret = true;

		break;
	}

	return ret;
}

Object* Map::GetObjectById(int _id)
{
	ListItem<ObjectLayer*>* objectLayer;
	objectLayer = mapData.objectLayers.start;
	while (objectLayer != NULL)
	{
		ListItem<Object*>* object;
		object = objectLayer->data->objects.start;
		while (object != NULL)
		{
			if (object->data->id == _id)
			{
				return object->data;
			}
			object = object->next;
		}
		objectLayer = objectLayer->next;
	}
	return nullptr;
}

//Pick the right Tileset based on a tile id
TileSet* Map::GetTilesetFromTileId(int id) const
{
	ListItem<TileSet*>* item = mapData.tilesets.start;
	TileSet* set = item->data;

	while (item)
	{
		if (id < item->data->firstgid)
		{
			set = item->prev->data;
			break;
		}
		set = item->data;
		item = item->next;
	}

	return set;
}

// Get relative Tile rectangle
SDL_Rect TileSet::GetTileRect(int id) const
{
	SDL_Rect rect = { 0 };

	//Get relative Tile rectangle
	int relativeId = id - firstgid;
	rect.w = tileWidth;
	rect.h = tileHeight;
	rect.x = margin + ((rect.w + spacing) * (relativeId % columns));
	rect.y = margin + ((rect.h + spacing) * (relativeId / columns));
	
	return rect;
}

// Called before quitting
bool Map::CleanUp()
{
    LOG("Unloading map");

    // clean up any memory allocated from tilesets/map
    // Remove all tilesets
	ListItem<TileSet*>* item;
	item = mapData.tilesets.start;

	while (item != NULL)
	{
		RELEASE(item->data);
		item = item->next;
	}
	mapData.tilesets.clear();

	// clean up all layer data
	// Remove all layers
	ListItem<MapLayer*>* item2;
	item2 = mapData.layers.start;

	while (item2 != NULL)
	{
		RELEASE(item2->data);
		item2 = item2->next;
	}
	mapData.layers.clear();

	// Remove all objects
	ListItem<ObjectLayer*>* item3;
	item3 = mapData.objectLayers.start;

	while (item3 != NULL)
	{
		RELEASE(item3->data);
		item3 = item3->next;
	}

	mapData.objectLayers.clear();

    return true;
}

// Load new map
bool Map::Load(const char* filename)
{
    bool ret = true;
    SString tmp("%s%s", folder.GetString(), filename);

	pugi::xml_document mapFile; 
    pugi::xml_parse_result result = mapFile.load_file(tmp.GetString());

    if(result == NULL)
    {
        LOG("Could not load map xml file %s. pugi error: %s", filename, result.description());
        ret = false;
    }

	// Load general info
    if(ret == true)
    {
        // Create and call a private function to load and fill all your map data
		ret = LoadMap(mapFile);
	}

    // Create and call a private function to load a tileset
    // remember to support more any number of tilesets!
	if (ret == true)
	{
		ret = LoadTileSets(mapFile.child("map"));

		if (ret)
			LOG("TileSets Loaded...");
		else
			LOG("TileSets Not Loaded...");
	}

	// Iterate all layers and load each of them
	// Load layer info
	if (ret == true)
	{
		ret = LoadAllLayers(mapFile.child("map"));
		if (ret)
			LOG("Layers Loaded...");
		else
			LOG("Layers Not Loaded...");
	}
    
	if (ret == true)
	{
		ret = LoadAllObjectLayers(mapFile.child("map"));
		if (ret)
			LOG("Objects Loaded...");
		else
			LOG("Objects Not Loaded...");
	}


    if(ret == true)
    {
        //LOG all the data loaded iterate all tilesets and LOG everything

		//LOG the info for each loaded layer
    }

	//once the maps and layers are loaded, we set the physics properties
	if (ret == true)
	{
		//here we will set ground and death colliders (water, spkies?)
		SetMapColliders();
	}

    mapLoaded = ret;

    return ret;
}

//Load map general properties
bool Map::LoadMap(pugi::xml_node mapFile)
{
	bool ret = true;
	pugi::xml_node map = mapFile.child("map");

	if (map == NULL)
	{
		LOG("Error parsing map xml file: Cannot find 'map' tag.");
		ret = false;
	}
	else
	{
		// Load map general properties
		mapData.height = map.attribute("height").as_int();
		mapData.width = map.attribute("width").as_int();
		mapData.tileHeight = map.attribute("tileheight").as_int();
		mapData.tileWidth = map.attribute("tilewidth").as_int();

		// Add formula to go from isometric map to world coordinates
		mapData.type = MAPTYPE_UNKNOWN;
		if (strcmp(map.attribute("orientation").as_string(), "isometric") == 0)
		{
			mapData.type = MAPTYPE_ISOMETRIC;
		}
		if (strcmp(map.attribute("orientation").as_string(), "orthogonal") == 0)
		{
			mapData.type = MAPTYPE_ORTHOGONAL;
		}
	}

	return ret;
}

// Implement the LoadTileSet function to load the tileset properties
bool Map::LoadTileSets(pugi::xml_node mapFile) {

	bool ret = true;

	pugi::xml_node tileset;
	for (tileset = mapFile.child("tileset"); tileset && ret; tileset = tileset.next_sibling("tileset"))
	{
		TileSet* set = new TileSet();
		if (ret == true) ret = LoadTilesetDetails(tileset, set);
		if (ret == true) ret = LoadTilesetImage(tileset, set);
		mapData.tilesets.add(set);
	}

	return ret;
}

// Load Tileset attributes
bool Map::LoadTilesetDetails(pugi::xml_node& tileset_node, TileSet* set)
{
	bool ret = true;

	// L03: DONE 4: Load Tileset attributes
	set->name.Create(tileset_node.attribute("name").as_string());
	set->firstgid = tileset_node.attribute("firstgid").as_int();
	set->tileWidth = tileset_node.attribute("tilewidth").as_int();
	set->tileHeight = tileset_node.attribute("tileheight").as_int();
	set->margin = tileset_node.attribute("margin").as_int();
	set->spacing = tileset_node.attribute("spacing").as_int();
	set->tilecount = tileset_node.attribute("tilecount").as_int();
	set->columns = tileset_node.attribute("columns").as_int();

	return ret;
}

// Load Tileset image
bool Map::LoadTilesetImage(pugi::xml_node& tileset_node, TileSet* set)
{
	bool ret = true;
	pugi::xml_node image = tileset_node.child("image");
	set->source = tileset_node.child("image").attribute("source").as_string();
	set->texHeight = tileset_node.child("image").attribute("tex_height").as_int();
	set->texWidth = tileset_node.child("image").attribute("text_width").as_int();

	if (image == NULL)
	{
		LOG("Error parsing tileset xml file: Cannot find 'image' tag.");
		ret = false;
	}
	else
	{
		// Load Tileset image
		SString tmp("%s%s", folder.GetString(), image.attribute("source").as_string());
		set->texture = app->tex->Load(tmp.GetString());
	}

	return ret;
}

// Iterate all layers and load each of them
bool Map::LoadAllLayers(pugi::xml_node mapNode) {
	bool ret = true;
	for (pugi::xml_node layerNode = mapNode.child("layer"); layerNode && ret; layerNode = layerNode.next_sibling("layer"))
	{
		//Load the layer
		MapLayer* mapLayer = new MapLayer();
		ret = LoadLayer(layerNode, mapLayer);

		//add the layer to the map
		mapData.layers.add(mapLayer);
	}

	return ret;
}

// Implement a function that loads a single layer layer
bool Map::LoadLayer(pugi::xml_node& node, MapLayer* layer)
{
	bool ret = true;

	//Load the attributes
	layer->name = node.attribute("name").as_string();
	layer->width = node.attribute("width").as_int();
	layer->height = node.attribute("height").as_int();

	//Call Load Propoerties
	LoadProperties(node, layer->properties);

	//Reserve the memory for the tile array
	layer->data = new uint[layer->width * layer->height];
	memset(layer->data, 0, layer->width * layer->height);

	//Iterate over all the tiles and assign the values
	pugi::xml_node tile;
	int i = 0;
	for (tile = node.child("data").child("tile"); tile && ret; tile = tile.next_sibling("tile"))
	{
		layer->data[i] = tile.attribute("gid").as_int();
		i++;
	}

	return ret;
}

//Load a group of properties from a node and fill a list with it
bool Map::LoadProperties(pugi::xml_node& node, Properties& properties)
{
	bool ret = true;

	for (pugi::xml_node propertieNode = node.child("properties").child("property"); propertieNode; propertieNode = propertieNode.next_sibling("property"))
	{
		Properties::Property* p = new Properties::Property();
		p->name = propertieNode.attribute("name").as_string();
		p->value = propertieNode.attribute("value").as_int();

		properties.list.add(p);
	}

	return ret;
}

bool Map::LoadAllObjectLayers(pugi::xml_node mapNode)
{
	bool ret = true;
	LOG("LOADING OBJECT LAYERS....");
	for (pugi::xml_node layerNode = mapNode.child("objectgroup"); layerNode && ret; layerNode = layerNode.next_sibling("objectgroup"))
	{
		//Load the layer
		ObjectLayer* objectLayer = new ObjectLayer();
		ret = LoadObjectLayer(layerNode, objectLayer);

		//add the layer to the map
		mapData.objectLayers.add(objectLayer);
	}

	return ret;
}

bool Map::LoadObjectLayer(pugi::xml_node& node, ObjectLayer* layer)
{
	bool ret = true;

	//Load object group attributes
	layer->name = node.attribute("name").as_string();

	//Check what type of object is 
	if (strcmp(layer->name.GetString(), "Gems") == 0)
	{
		layer->texture = app->tex->Load("../Output/Assets/Spritesx16/gems.png");
		if (layer->texture == NULL)
			LOG("Gems texture not loaded...");

	}else if (strcmp(layer->name.GetString(), "checkpoints" )== 0)
	{
		layer->texture = NULL;

	}else if (strcmp(layer->name.GetString(), "Potions" )== 0)
	{
		layer->texture = layer->texture = app->tex->Load("../Output/Assets/Spritesx16/props.png");;
	}


	LOG("LOADING OBJECT LAYER....");
	//Create and load each object property
	pugi::xml_node object;
	LOG("node: %s", node.name());
	for (object = node.child("object"); object && ret; object = object.next_sibling("object"))
	{
		LOG("node: %s", object.name());

		Object* obj = new Object();
		//store object attributes 
		obj->name = object.attribute("name").as_string();
		obj->id = object.attribute("id").as_int();
		obj->x = object.attribute("x").as_int();
		obj->y = object.attribute("y").as_int();
		obj->width = object.attribute("width").as_int();
		obj->height = object.attribute("height").as_int();

		LOG("OBJECT ID: %i", obj->id);
		//Check what type of object is 

		if (strcmp(object.attribute("type").as_string(), "Player") == 0) {

			obj->type = Collider_Type::PLAYER;

		}else if (strcmp(object.attribute("type").as_string(), "Gem") == 0){

			obj->type = Collider_Type::GEM;

		}else if (strcmp(object.attribute("type").as_string(), "Key") == 0){

			obj->type = Collider_Type::KEY;
			
		}else if (strcmp(object.attribute("type").as_string(), "HPotion") == 0){

			obj->type = Collider_Type::POTION;
			
		}else if (strcmp(object.attribute("type").as_string(), "Portal") == 0){

			obj->type = Collider_Type::PORTAL;

		}else if (strcmp(object.attribute("type").as_string(), "Checkpoint") == 0) {

			obj->type = Collider_Type::CHECK_POINT;

		}else if (strcmp(object.attribute("type").as_string(), "Bat") == 0) {

			obj->type = Collider_Type::BAT;
		}
		else if (strcmp(object.attribute("type").as_string(), "Musher") == 0) {

			obj->type = Collider_Type::MUSHER;
		}
		else if (strcmp(object.attribute("type").as_string(), "BigMusher") == 0) {

			obj->type = Collider_Type::BIG_MUSHER;
		}
		else if (strcmp(object.attribute("type").as_string(), "Ground") == 0)
		{
			obj->type = Collider_Type::GROUND;
		}

		layer->objects.add(obj);
		//send current object node and obj to store the properties
		LoadObject(object, obj);
	}
	
	return ret;
}

bool Map::LoadObject(pugi::xml_node& node, Object* object)
{
	bool ret = true;

	//Iterate over all the object properties and set values, store the object to the list
	LOG("LOADING OBJECTS....");
	pugi::xml_node objProperty;
	int i = 0;
	for (objProperty = node.child("properties").child("property"); objProperty && ret; objProperty = objProperty.next_sibling("property"))
	{
		Properties::Property* p = new Properties::Property();

		p->name = objProperty.attribute("name").as_string();
		p->value = objProperty.attribute("value").as_int();

		object->properties.list.add(p);
	}

	return ret;
}


bool Map::SetMapColliders()
{
	bool ret = true;

	//ListItem<MapLayer*>* mapLayerItem;
	//mapLayerItem = mapData.layers.start;
	//LOG("--------!!!SETTING COLLIDERS!!!---------");
	//while (mapLayerItem != NULL) {

	//	if (mapLayerItem->data->properties.GetProperty("Collider") == 1) {

	//		for (int x = 0; x < mapLayerItem->data->width; x++)
	//		{
	//			for (int y = 0; y < mapLayerItem->data->height; y++)
	//			{
	//				int gid = mapLayerItem->data->Get(x, y);

	//				if (gid > 0) {

	//					TileSet* tileset = GetTilesetFromTileId(gid);

	//					SDL_Rect r = tileset->GetTileRect(gid);
	//					iPoint pos;
	//					pos = MapToWorld(x,y);
	//					PhysBody* pb = app->physics->CreateRectangle(pos.x + (tileset->tileWidth * 0.5f), pos.y + (tileset->tileHeight * 0.5f), tileset->tileWidth, tileset->tileHeight, b2_staticBody);
	//					pb->color = { 0,0,0,255 };
	//					pb->type = Collider_Type::GROUND;
	//					app->physics->allPhysicBodies.add(pb);
	//				}
	//			}
	//		}

	//	}else if (mapLayerItem->data->properties.GetProperty("Death") == 1) {

	//		for (int x = 0; x < mapLayerItem->data->width; x++)
	//		{
	//			for (int y = 0; y < mapLayerItem->data->height; y++)
	//			{
	//				int gid = mapLayerItem->data->Get(x, y);

	//				if (gid > 0) {

	//					TileSet* tileset = GetTilesetFromTileId(gid);

	//					SDL_Rect r = tileset->GetTileRect(gid);
	//					iPoint pos;
	//					pos = MapToWorld(x, y);

	//					PhysBody* pb = app->physics->CreateRectangle(pos.x + (tileset->tileWidth * 0.5f), pos.y + (tileset->tileHeight * 0.5f), tileset->tileWidth, tileset->tileHeight, b2_staticBody);
	//					pb->color = { 255,50,50,255 };
	//					pb->listener = app->levelManagement->currentScene;
	//					pb->type = Collider_Type::DEATH;
	//					app->physics->allPhysicBodies.add(pb);
	//				}

	//			}
	//		}
	//	}
	//	mapLayerItem = mapLayerItem->next;
	//}
		ListItem<ObjectLayer*>* objectLayer;
	objectLayer = mapData.objectLayers.start;
	LOG("--------!!!SETTING ENTITIES!!!---------");
	PhysBody* pb;

	while (objectLayer != NULL)
	{
	
		LOG("SETTING %s LAYER COLLIDER...",objectLayer->data->name.GetString());
		ListItem<Object*>* object;
		object = objectLayer->data->objects.start;
		while (object != NULL)
		{

			iPoint spawnPos;
			spawnPos.x = object->data->x + object->data->width * 0.5;
			spawnPos.y = object->data->y + object->data->height * 0.5;
			

			switch (object->data->type)
			{
	
				case PLAYER:
					app->entities->AddEntity(PLAYER, spawnPos);
					LOG("SPAWN PLAYER...");
					break;
				case BAT:
					app->entities->AddEntity(BAT, spawnPos);
					LOG("SPAWN BAT...");
					break;
				case MUSHER:
					app->entities->AddEntity(MUSHER, spawnPos);
					LOG("SPAWN MUSHER...");
					break;
				case BIG_MUSHER:
					app->entities->AddEntity(BIG_MUSHER, spawnPos);
					LOG("SPAWN BIG MUSHER...");
					break;
				case GEM:
					app->entities->AddEntity(Collider_Type::GEM, spawnPos);
					LOG("SETTING GEM COLLIDER...");
					break;
				case KEY:
					app->entities->AddEntity(Collider_Type::KEY, spawnPos);
					LOG("SETTING KEY COLLIDER...");
					break;
				case POTION:
					app->entities->AddEntity(Collider_Type::POTION, spawnPos);
					LOG("SETTING POTION COLLIDER...");
					break;
				case PORTAL:
					app->entities->AddEntity(Collider_Type::PORTAL, spawnPos);
					LOG("SETTING PORTAL COLLIDER...");
					break;
				case CHECK_POINT:
					app->entities->AddEntity(Collider_Type::CHECK_POINT, spawnPos);
					LOG("SETTING CHECKPOINT COLLIDER...");
					break;
				case GROUND:
					pb = app->physics->CreateRectangle(spawnPos.x, spawnPos.y +16 ,object->data->width, object->data->height, b2_staticBody);
					pb->color = { 0,0,0,255 };
					pb->type = Collider_Type::GROUND;
					app->physics->allPhysicBodies.add(pb);
					break;
				case DEATH:
					break;
				default:
					break;
			}

			object = object->next;

		}

		objectLayer = objectLayer->next;
	}

	return ret;
}

void Map::ClearColliders()
{
	app->physics->allPhysicBodies.clear();
}

bool Map::LoadState(pugi::xml_node& data)
{
	bool ret = true;
	return ret;
}

bool Map::SaveState(pugi::xml_node& data) const
{

	bool ret = true;
	return ret;
}