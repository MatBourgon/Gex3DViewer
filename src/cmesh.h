#pragma once
#include "level.h"

// custom mesh stuff

std::shared_ptr<Model> CreatePathPointObject(level_t& level, addr_t addr);

void CreateCircleMesh(std::shared_ptr<Model> model, float r, glm::u8vec3 color, int height, int p = 12);

void AddDiamondToModel(std::shared_ptr<Model> model, glm::vec3 pos, float scale = 1.f);

void AddLineToModel(std::shared_ptr<Model> model, glm::vec3 start, glm::vec3 end);

void CreateSpriteObject(level_t& level, std::shared_ptr<Model> model, const std::string& name, unsigned int customId, int scale = 5);

void ApplyPathModels(level_t& level);

namespace ECustomImageType
{
	enum
	{
		CUSTOM_IMAGE_BASE = 0x100000,
		INFO_SPAWN = CUSTOM_IMAGE_BASE + 0,
		INFO_PROXSIG,
		INFO_UNKNOWN,
		INFO_POINT,
		INFO_COLD,
		INFO_EMPTY
	};
}

ImagePacker::ImageInformation_t* FindImageInfoById(ImagePacker::ImageInformationList& list, int id);