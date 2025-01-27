#pragma once

#include "imagepacker.h"
#include "components/IComponent.h"
#include "components/ScriptComponent.h"
#include "components/PathComponent.h"

#include <vector>
#include <memory>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <string>
#include <map>

/* INSTANCE FLAGS
  0x01: ???
  0x02: ???
  0x04: Part of destruction objective; needs to be in list
*/

struct objinstance_t
{
	glm::vec3 position{ 0, 0, 0 };
	glm::vec3 rotation{ 0, 0, 0 };
	bool isVisible = true;
	unsigned int address = 0;
	unsigned int flags = 0;
	unsigned int instanceData[4] = { 0 };


	glm::vec3 oposition{ 0, 0, 0 };
	glm::vec3 orotation{ 0, 0, 0 };

	std::vector<std::unique_ptr<IComponent>> components;
	template<typename T>
	T& AddComponent()
	{
		components.push_back(std::make_unique<T>());
		return *(T*)components.back().get();
	}
	template<typename T>
	T* GetComponent()
	{
		for (auto& c : components)
		{
			if (auto p = dynamic_cast<T*>(c.get()))
				return p;
		}

		return nullptr;
	}
};

struct Model
{
	struct vertex_t
	{
		int x, y, z;
		int oX, oY, oZ;
		unsigned short normalId;
		unsigned char r, g, b, a;
	};
	struct polygon_t
	{
		size_t vertex[3];
		unsigned int materialID;
		unsigned short flags;
		glm::vec2 uvs[3];
		unsigned char optColors[4] = { 0, 0, 0, 0 };
		bool isTrigger = false;
	};
	const unsigned int addr;
	std::string name;
	std::vector<vertex_t> vertices;
	std::vector<polygon_t> polygons;
	std::vector<objinstance_t> instances;
	bool objectVisibility = true;
	bool showInstances = false;
	bool hasNoTextures = false;

	Model(unsigned int addr) : addr(addr) {}
};

struct texture_t
{
	unsigned int w, h;
	glm::vec4* pixels;
	bool deletePixels = true;
	bool argb1555 = false;
};

struct level_t
{
	std::vector<std::shared_ptr<Model>> models;
	std::vector<texture_t> textures;
	ImagePacker::ImageInformationList list;
	texture_t sheet{ 0, 0, NULL };
	std::string name;
	float bgColor[3];
	char pickupName[3][9];
	unsigned int baseData;

	std::map<unsigned int, ScriptComponent> signals;
};

inline std::string Hexify(unsigned int n)
{
	if (n == 0)
		return "0";
	std::string s;
	while (n > 0)
	{
		if (n % 0x10 > 9)
		{
			s += 'a' + ((n % 0x10) - 10);
		}
		else
		{
			s += '0' + (n % 0x10);
		}
		n >>= 4;
	}
	std::reverse(s.begin(), s.end());
	return s;
};