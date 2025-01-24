#pragma once
#include "IComponent.h"

#include <vector>

struct PathVector3
{
	unsigned short speed;
	short x, y, z;
};

struct PathQuaternion
{
	unsigned int speed;
	float rotX, rotY, rotZ, rotW;
};

struct Path
{
	unsigned int address;
	unsigned int owner;
	std::vector<PathVector3> translations;
	std::vector<PathQuaternion> rotations;
	std::vector<PathVector3> scalars;
};

// Maybe move to own file to de-clutter
struct PathComponent : public IComponent
{
	Path path;

	virtual void ParseData(file_t& file, level_t& level, unsigned int data) override;
	virtual void ExportData(std::stringstream& ss) override;
};