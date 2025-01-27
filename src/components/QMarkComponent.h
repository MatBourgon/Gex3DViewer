#pragma once
#include "IComponent.h"

#include <vector>

struct QMarkComponent : public IComponent
{
	struct QMarkEntry
	{
		unsigned short xPos, yPos;
		std::string messageRaw;
		std::string message;
	};
	unsigned short x, y, w, h;
	unsigned short length;
	bool showRaw = false;
	std::vector<QMarkEntry> entries;
	virtual void ParseData(file_t& file, level_t& level, unsigned int data) override;
	virtual void ExportData(JSON& object) override;
	virtual void RenderGUI(level_t& level, void* textureSheet) override;
	static bool HasComponent(const std::string& modelName) { return modelName == "qmark___"; }
};