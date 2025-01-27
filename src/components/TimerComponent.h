#pragma once
#include "IComponent.h"

struct TimerComponent : public IComponent
{
	unsigned int missionTime; // seconds
	unsigned int cutsceneTime; // frames? ticks?
	virtual void ParseData(file_t& file, level_t& level, unsigned int data) override;
	virtual void ExportData(JSON& object) override;
	virtual void RenderGUI(level_t& level, void* textureSheet) override;
	static bool HasComponent(const std::string& modelName) { return modelName == "btimer__"; }
};