#pragma once
#include "IComponent.h"

struct FlyBoxComponent : public IComponent
{
	unsigned char flyBoxType;
	bool respawns;

	virtual void ParseData(file_t& file, level_t& level, unsigned int data) override;
	virtual void ExportData(std::stringstream& ss) override;
	virtual void RenderGUI(level_t& level, void* textureSheet) override;
	static bool HasComponent(const std::string& modelName)
	{
		return modelName == "circitv_" || modelName == "powertv_";
	}
};