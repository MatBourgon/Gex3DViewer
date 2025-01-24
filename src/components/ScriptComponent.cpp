#include "ScriptComponent.h"
#include "../script.h"

#include <imgui/imgui.h>

void ScriptComponent::ParseData(file_t& file, level_t& level, unsigned int data)
{
	if (data == 0)
		return;

	addr_t addr = file.ReadAt<addr_t>(data);
	if (addr != 0)
	{
		ParseCommands(level, file, addr, commands);
	}
}

void ScriptComponent::ExportData(std::stringstream& ss)
{
	ss << "{ \"component_type\": \"script\" }"; // todo: add script
}

void ScriptComponent::RenderGUI(level_t& level, void* textureSheet)
{
	ImGui::Text("Commands:");
	ImGui::BeginGroup();
	ImGui::Indent(8.f);
	for (auto& c : commands)
	{
		ImGui::Text(c.c_str());
	}
	ImGui::EndGroup();
}

bool ScriptComponent::HasComponent(const std::string& modelName)
{
	const static std::vector<std::string> types = {
		"hrswtch_",
		"bell____",
		"aztcbks_",
		"cart____",
		"rezcrnk_",
		"ninja___"
	};
	return std::find_if(types.begin(), types.end(), [&modelName](const std::string& s) { return s == modelName; }) != types.end();
}
