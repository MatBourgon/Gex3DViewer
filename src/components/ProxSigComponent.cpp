#include "ProxSigComponent.h"
#include "../script.h"
#include "../json.h"

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

void ProxSigComponent::ExportData(JSON& object)
{
	object["component_type"] = "proxsig";
	object["proxies"] = JSON::Array();
	for (auto& p : proxies)
	{
		JSON jo;
		jo["range"] = { p.rangeMin, p.rangeMax };
		// todo: add script
		object["proxies"].push_back(jo);
	}
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