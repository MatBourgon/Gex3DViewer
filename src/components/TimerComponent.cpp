#include "TimerComponent.h"

#include <imgui/imgui.h>

void TimerComponent::ParseData(file_t& file, level_t& level, unsigned int data)
{
	if (data != 0)
	{
		missionTime = file.ReadAt<unsigned short>(data);
		cutsceneTime = file.ReadAt<unsigned short>(data + 2);
	}
	else
		missionTime = cutsceneTime = 0;
}

void TimerComponent::ExportData(std::stringstream& ss)
{
	ss << "{ \"component_type\": \"timer\", \"mission_time\": " << std::to_string(missionTime) << ", \"cutscene_time\": " << std::to_string(cutsceneTime) << " }";
}

void TimerComponent::RenderGUI(level_t& level, void* textureSheet)
{
	ImGui::Text("Timer: %d:%.2d", missionTime / 60, missionTime % 60);
	ImGui::Text("Stall Time: %d", cutsceneTime);
}