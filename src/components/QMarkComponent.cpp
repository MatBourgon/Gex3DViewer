#include "QMarkComponent.h"
#include "../json.h"

#include <imgui/imgui.h>

void QMarkComponent::ParseData(file_t& file, level_t& level, unsigned int data)
{
	file.seek(data);
	x = file.Read<u16>(0, true);
	y = file.Read<u16>(0, true);
	w = file.Read<u16>(0, true);
	h = file.Read<u16>(0, true);
	length = file.Read<u16>(0, true);
	u16 nStrs = file.Read<u16>(0, true);

	for (u16 i = 0; i < nStrs; ++i)
	{
		entries.push_back({});
		entries.back().xPos = file.Read<u16>(0, true);
		entries.back().yPos = file.Read<u16>(0, true);
		addr_t strAddr = file.Read<addr_t>(0, true);
		int j = 0;
		while (file.ReadAt<char>(strAddr + j) != '\0') ++j;
		entries.back().messageRaw.resize(j, '\0');
		memcpy(entries.back().messageRaw.data(), file.ptrAt(strAddr), j);
		entries.back().message = entries.back().messageRaw;
		for (auto& c : entries.back().message)
			if (c == '_')
				c = ' ';
	}

	file.pop();
}

void QMarkComponent::ExportData(JSON& object)
{
	object["component_type"] = "qmark";
	object["box"] = { x, y, w, h };
	object["length"] = length;
	object["messages"] = JSON::Array();

	for (auto& e : entries)
	{
		JSON jo;
		jo["x"] = e.xPos;
		jo["y"] = e.yPos;
		jo["text"] = e.messageRaw;
		object["messages"].push_back(jo);
	}
}

void QMarkComponent::RenderGUI(level_t& level, void* textureSheet)
{
	ImGui::Text("Box Position: (%d, %d)", x, y);
	ImGui::Text("Box Size: (%d, %d)", w, h);
	ImGui::Text("Message Time: %d", length);
	ImGui::Checkbox(("Show Raw Text?##" + std::to_string((size_t)this)).c_str(), &showRaw);
	ImGui::BeginGroup();
	ImGui::Indent();
	for (auto& m : entries)
	{
		ImGui::Text("(%d, %d): \"%s\"", m.xPos, m.yPos, (showRaw ? m.messageRaw : m.message).c_str());
	}
	ImGui::EndGroup();
}