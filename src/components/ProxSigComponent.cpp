#include "ProxSigComponent.h"
#include "../script.h"

#include <imgui/imgui.h>

void ProxSigComponent::ParseData(file_t& file, level_t& level, unsigned int data)
{
	if (data == 0)
		return;

	file.seek(data);
	int nScripts = file.Read<unsigned int>(0, true);
	for (int i = 0; i < nScripts; ++i)
	{
		proxies.push_back({});
		auto& proxy = proxies.back();
		proxy.rangeMin = file.Read<unsigned short>(0, true);
		proxy.rangeMax = file.Read<unsigned short>(0, true);
		ParseCommands(level, file, file.Read<addr_t>(0, true), proxy.commands);
	}
	file.pop();
}

void ProxSigComponent::ExportData(std::stringstream& ss)
{
	ss << "{ \"component_type\": \"proxsig\", \"proxies\": [";
	for (size_t i = 0; i < proxies.size(); ++i)
	{
		ss << "{ \"range\": [" << std::to_string(proxies[i].rangeMin) << ", " << std::to_string(proxies[i].rangeMax) << "] }"; // todo: add script
		if ((i + 1) < proxies.size())
			ss << ", ";
	}
	ss << " ] }";
}

void ProxSigComponent::RenderGUI(level_t& level, void* textureSheet)
{
	ImGui::Text("Proxies:");
	for (auto& proxy : proxies)
	{
		ImGui::BeginGroup();
		ImGui::Indent(8.f);
		ImGui::Separator();
		ImGui::Text("Range: %d <= distance <= %d", proxy.rangeMin, proxy.rangeMax);
		for (auto& s : proxy.commands)
		{
			ImGui::Text(s.c_str());
		}
		ImGui::EndGroup();
	}
}