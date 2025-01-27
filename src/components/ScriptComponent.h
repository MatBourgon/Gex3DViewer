#pragma once
#include "IComponent.h"

#include <vector>

struct ScriptComponent : public IComponent
{
	std::vector<std::string> commands;

	virtual void ParseData(file_t& file, level_t& level, unsigned int data) override;
	virtual void ExportData(JSON& object) override;
	virtual void RenderGUI(level_t& level, void* textureSheet) override;
	static bool HasComponent(const std::string& modelName);
};