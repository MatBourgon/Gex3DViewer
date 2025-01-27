#include "TimerComponent.h"

#include "../json.h"

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

void TimerComponent::ExportData(JSON& object)
{
	object["component_type"] = "timer";
	object["mission_type"] = missionTime;
	object["cutscene_time"] = cutsceneTime;
}

void TimerComponent::RenderGUI(level_t& level, void* textureSheet)
{
	ImGui::Text("Timer: %d:%.2d", missionTime / 60, missionTime % 60);
	ImGui::Text("Stall Time: %d", cutsceneTime);
}