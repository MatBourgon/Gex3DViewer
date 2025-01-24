#pragma once
#include "../file.h"

#include <sstream>

struct level_t;

struct IComponent
{
	~IComponent() = default;

	virtual void ParseData(file_t& file, level_t& level, unsigned int data) = 0;
	virtual void ExportData(std::stringstream& ss) = 0;
	virtual void RenderGUI(level_t& level, void* textureSheet) {}

protected:
	IComponent() = default;
};