#include "FlyBoxComponent.h"
#include "../level.h"
#include "../script.h"

#include <imgui/imgui.h>

void FlyBoxComponent::ParseData(file_t& file, level_t& level, unsigned int data)
{
	if (data == 0)
	{
		flyBoxType = 0;
		respawns = false;
	}
	else
	{
		flyBoxType = file.ReadAt<byte>(data);
		respawns = file.ReadAt<byte>(data + 2);
		if (flyBoxType == 5) // checkpoint
		{
			addr_t signal = file.Read<addr_t>(data + 4);
			if (!level.signals.contains(signal))
			{
				ParseCommands(level, file, signal, level.signals[signal].commands);
				if (!level.signals[signal].commands.empty())
					level.signals[signal].commands[0] += "#Checkpoint";
			}
		}
	}
}

void FlyBoxComponent::ExportData(std::stringstream& ss)
{
	ss << "{ \"component_type\": \"fly_box\", \"type\": " << std::to_string(flyBoxType) << ", \"respawns\": " << (respawns ? "true" : "false") << " }";
}

void FlyBoxComponent::RenderGUI(level_t& level, void* textureSheet)
{
	static const char* const c_FlyTypeText[] = {
		"Health",
		"Fire",
		"Ice",
		"Unused",
		"Extra Life",
		"Checkpoint"
	};

	ImGui::Text("Fly type: %s", c_FlyTypeText[flyBoxType]);
	ImGui::Text("Respawns: %s", respawns ? "Yes" : "No");
}