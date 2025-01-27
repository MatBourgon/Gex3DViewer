#include "EndTVComponent.h"
#include "../json.h"

#include <imgui/imgui.h>

void EndTVComponent::ParseData(file_t& file, level_t& level, unsigned int data)
{
	file.seek(data);
	remoteId = file.Read<byte>(0);
	always3 = file.Read<unsigned short>(2); // no clue why, maybe used to be the # of remotes in a level?
	condition = file.Read<unsigned int>(4);
	file.pop();
}

void EndTVComponent::ExportData(JSON& object)
{
	object["component_type"] = "end_tv";
	object["remote_id"] = remoteId;
}

void EndTVComponent::RenderGUI(level_t& level, void* textureSheet)
{
	ImGui::Text("Remote ID: %d", remoteId);
	ImGui::Text("Always 3: %d", always3);
	ImGui::Text("Condition: %d", condition);
}

bool EndTVComponent::HasComponent(const std::string& modelName)
{
	return modelName.starts_with("endtv");
}
