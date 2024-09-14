#pragma once
#include <string>
#include <vector>
#include <memory>
#include <glm/vec3.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

struct Model
{
	struct vertex_t
	{
		short x, y, z;
		unsigned short normalId;
		unsigned char r, g, b, a;
	};
	struct polygon_t
	{
		size_t vertex[3];
		unsigned int materialID;
		unsigned short flags;
		glm::vec2 uvs[3];
	};
	const std::string name;
	std::vector<vertex_t> vertices;
	std::vector<polygon_t> polygons;
};

struct texture_t
{
	unsigned int w, h;
	glm::vec4* pixels;
};

struct level_t
{
	std::vector<std::shared_ptr<Model>> models;
	std::vector<texture_t> textures;
};

bool LoadLevel(const std::string& filepath, level_t& level);