#include "PathComponent.h"

#include "../level.h"
#include "../json.h"

#include <glm/ext/scalar_constants.hpp>

void ReadMovingPlatform(Path& path, file_t& dfx, level_t& level, addr_t ownerAddr, addr_t platformAddr)
{
	path.address = platformAddr;
	dfx.seek(platformAddr);
	addr_t translationAddr = dfx.Read<addr_t>(0);
	addr_t rotationAddr = dfx.Read<addr_t>(4);
	addr_t scaleAddr = dfx.Read<addr_t>(8);
	if (platformAddr == 0)
	{
		dfx.pop();
		return;
	}

	if (translationAddr != 0)
	{
		dfx.seek(translationAddr, true);
		addr_t pointsAddr = dfx.Read<addr_t>(0);
		u16 nPoints = dfx.Read<u16>(4);
		dfx.seek(pointsAddr, true);
		for (u16 i = 0; i < nPoints; ++i)
		{
			path.translations.push_back({
				dfx.Read<u16>(i * 0x20 + 0),
				dfx.Read<i16>(i * 0x20 + 2),
				dfx.Read<i16>(i * 0x20 + 4),
				dfx.Read<i16>(i * 0x20 + 6)
				});
		}
	}

	if (rotationAddr != 0)
	{
		dfx.seek(rotationAddr, true);
		u16 nRots = dfx.Read<u16>(4);
		dfx.seek(dfx.Read<addr_t>(0), true);
		for (u16 i = 0; i < nRots; ++i)
		{
			path.rotations.push_back({
				dfx.Read<u16>(i * 10 + 0),
				dfx.Read<i16>(i * 10 + 2) * (1.f / 0x1000),
				dfx.Read<i16>(i * 10 + 4) * (1.f / 0x1000),
				dfx.Read<i16>(i * 10 + 6) * (1.f / 0x1000),
				dfx.Read<i16>(i * 10 + 8) * (-1.f / 0x1000)
				});
		}
	}

	if (scaleAddr != 0)
	{
		dfx.seek(scaleAddr, true);
		addr_t pointsAddr = dfx.Read<addr_t>(0);
		u16 nPoints = dfx.Read<u16>(4);
		dfx.seek(pointsAddr, true);
		for (u16 i = 0; i < nPoints; ++i)
		{
			path.scalars.push_back({
				dfx.Read<u16>(i * 0x20 + 0),
				dfx.Read<i16>(i * 0x20 + 2),
				dfx.Read<i16>(i * 0x20 + 4),
				dfx.Read<i16>(i * 0x20 + 6)
				});
		}
	}

	dfx.pop();
}

void PathComponent::ParseData(file_t& file, level_t& level, unsigned int data)
{
	ReadMovingPlatform(path, file, level, data, data);
}

void PathComponent::ExportData(JSON& object)
{
	object["component_type"] = "spline";

	object["translations"] = JSON::Array();
	for (auto& pt : path.translations)
	{
		JSON jo;
		jo["speed"] = pt.speed;
		jo["pos"] = {
			pt.x, pt.y, pt.z
		};
		object["translations"].push_back(jo);
	}

	object["rotations"] = JSON::Array();
	for (auto& pt : path.rotations)
	{
		JSON jo;
		jo["speed"] = pt.speed;
		jo["rot"] = {
			pt.rotX, pt.rotY, pt.rotZ, pt.rotW
		};
		object["rotations"].push_back(jo);
	}

	object["scalars"] = JSON::Array();
	for (auto& pt : path.translations)
	{
		JSON jo;
		jo["speed"] = pt.speed;
		jo["scale"] = {
			pt.x, pt.y, pt.z
		};
		object["scalars"].push_back(jo);
	}
}
