#pragma once
#include "IComponent.h"

#include <vector>

struct ProxSigComponent : public IComponent
{
	struct Proximity
	{
		short rangeMin, rangeMax;
		std::vector<std::string> commands;
	};
	std::vector<Proximity> proxies;
	virtual void ParseData(file_t& file, level_t& level, unsigned int data) override;
	virtual void ExportData(std::stringstream& ss) override;
	virtual void RenderGUI(level_t& level, void* textureSheet) override;
	static bool HasComponent(const std::string& modelName) { return modelName == "proxsig_"; }
};