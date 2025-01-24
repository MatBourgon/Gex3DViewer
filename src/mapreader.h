#pragma once
#include "level.h"

#include <string>

bool LoadLevel(const std::string& filepath, level_t& level);

void AddLineToModel(std::shared_ptr<Model> model, glm::vec3 start, glm::vec3 end);