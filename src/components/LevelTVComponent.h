#pragma once
#include "IComponent.h"

struct LevelTVComponent : public IComponent
{
	char levelType[9]{ 0 };
	unsigned char levelNum;
	unsigned char screenType;
	virtual void ParseData(file_t& file, level_t& level, unsigned int data) override;
	virtual void ExportData(std::stringstream& ss) override;
	virtual void RenderGUI(level_t& level, void* textureSheet) override;
	static bool HasComponent(const std::string& modelName) { return modelName == "lvltv___"; }
};