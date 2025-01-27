#pragma once
#include "IComponent.h"

struct EndTVComponent : public IComponent
{
	unsigned char remoteId;
	unsigned short always3;
	unsigned int condition;
	virtual void ParseData(file_t& file, level_t& level, unsigned int data) override;
	virtual void ExportData(JSON& object) override;
	virtual void RenderGUI(level_t& level, void* textureSheet) override;
	static bool HasComponent(const std::string& modelName);
};