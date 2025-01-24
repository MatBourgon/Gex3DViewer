#include "PathComponent.h"

#include "../level.h"

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
		constexpr float c_PI_2_FROM_1024 = glm::pi<float>() / 2048.f;
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

void PathComponent::ExportData(std::stringstream& ss)
{
	ss << "{ \"component_type\": \"spline\", ";
	ss << "\"translations\":[";
	for (auto& pt : path.translations)
	{
		ss << " {\"speed\": " << std::to_string(pt.speed) << ", \"pos\": ["
			<< std::to_string(pt.x) << ", "
			<< std::to_string(pt.y) << ", "
			<< std::to_string(pt.z) << "]},";
	}
	if (path.translations.size() > 0)
		ss.seekp(-1, std::ios_base::end);
	ss << "], \"rotations\":[";
	for (auto& pt : path.rotations)
	{
		ss << " {\"speed\": " << std::to_string(pt.speed) << ", \"rot\": ["
			<< std::to_string(pt.rotX) << ", "
			<< std::to_string(pt.rotY) << ", "
			<< std::to_string(pt.rotZ) << ", "
			<< std::to_string(pt.rotW) << "]},";
	}
	if (path.rotations.size() > 0)
		ss.seekp(-1, std::ios_base::end);
	ss << "], \"scalars\":[";
	for (auto& pt : path.scalars)
	{
		ss << " {\"speed\": " << std::to_string(pt.speed) << ", \"scale\": ["
			<< std::to_string(pt.x / 4096.f) << ", "
			<< std::to_string(pt.y / 4096.f) << ", "
			<< std::to_string(pt.z / 4096.f) << "]},";
	}
	if (path.scalars.size() > 0)
		ss.seekp(-1, std::ios_base::end);
	ss << "] }";

}
