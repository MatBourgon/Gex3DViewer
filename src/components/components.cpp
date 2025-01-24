#include "components.h"
#include "IComponent.h"

#include "EndTVComponent.h"
#include "FlyBoxComponent.h"
#include "LevelTVComponent.h"
#include "PathComponent.h"
#include "ProxSigComponent.h"
#include "QMarkComponent.h"
#include "ScriptComponent.h"
#include "TimerComponent.h"

#include "../level.h"

#include <imgui/imgui.h>

template<typename T>
struct BasicValueComponent : public IComponent
{
	T value;
	const char* name;
	bool hex = false;

	virtual void ParseData(file_t& file, level_t& level, unsigned int data) override
	{
		if (data == 0)
			value = 0;
		else
			value = file.ReadAt<T>(data);
	}
	virtual void ExportData(std::stringstream& ss) override {}
	virtual void RenderGUI(level_t& level, void* textureSheet) override
	{
		if (hex)
			ImGui::Text("%s: %x", name, value);
		else
			ImGui::Text("%s: %d", name, value);
	}

};

struct IdentifierComponent : public IComponent
{
	unsigned int id;
	virtual void ParseData(file_t& file, level_t& level, unsigned int data) override
	{
		if (data == 0)
		{
			id = 0;
			return;
		}

		id = file.ReadAt<unsigned int>(data);
	}

	virtual void ExportData(std::stringstream& ss) override {}
	virtual void RenderGUI(level_t& level, void* textureSheet) override
	{
		if (id != 0)
		{
			ImGui::Text("ID: %x", id);
		}
	}
};

// There should only ever be one component for instance data 0
#define TRY_COMPONENT(Component)\
if (Component::HasComponent(model.name))\
{\
instance.AddComponent<Component>().ParseData(file, level, instance.instanceData[0]); \
return;\
}

void ReadComponents(file_t& file, level_t& level, Model& model)
{
	auto& instance = model.instances.back();
	if (instance.instanceData[2] != 0)
		instance.AddComponent<PathComponent>().ParseData(file, level, instance.instanceData[2]);

	TRY_COMPONENT(EndTVComponent);
	TRY_COMPONENT(FlyBoxComponent);
	TRY_COMPONENT(LevelTVComponent);
	TRY_COMPONENT(ProxSigComponent);
	TRY_COMPONENT(QMarkComponent);
	TRY_COMPONENT(ScriptComponent);
	TRY_COMPONENT(TimerComponent);
}
