#include "LevelTVComponent.h"
#include "../cmesh.h"

#include <imgui/imgui.h>

void LevelTVComponent::ParseData(file_t& file, level_t& level, unsigned int data)
{
	file.seek(data);
	screenType = file.Read<byte>(0);
	levelNum = file.Read<byte>(2);
	strncpy_s(levelType, file.ptr<char>() + 4, strnlen(file.ptr<char>() + 4, 8));
	file.pop();
}

void LevelTVComponent::ExportData(std::stringstream& ss)
{
	const char* c_ICON_NAME[] = {
		NULL, NULL, NULL, NULL, // these should never happen
		"GILLIGEX", "MOOSHOO", "GEXZILLA", "REZOPOLIS", "PREHISTORY", "TOON", "CIRCUIT", "SCREAM", "ROCKET", "KUNGFU", "CHANNELZ", "BONUS"
	};
	ss << "{ \"component_type\": \"level_tv\", \"level_id\": \"" << levelType << std::to_string(levelNum) << "\", \"tv_icon\": \"" << c_ICON_NAME[screenType] << "\" }";
}

void LevelTVComponent::RenderGUI(level_t& level, void* textureSheet)
{
	ImGui::Text("Level ID: %s%d", levelType, levelNum);

	if (auto info = FindImageInfoById(level.list, 200 + screenType))
	{
		ImGui::SameLine();
		ImGui::Image(textureSheet, { 16, 16 },
			{ info->x / (float)level.sheet.w, info->y / (float)level.sheet.h },
			{ (info->x + info->width) / (float)level.sheet.w, (info->y + info->height) / (float)level.sheet.h }
		);
	}
}