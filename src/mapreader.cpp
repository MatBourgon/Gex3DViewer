#include "mapreader.h"

#include "glideconstants.h"
#include "file.h"
#include "script.h"
#include "cmesh.h"
#include "components/components.h"
#include "components/EndTVComponent.h"
#include "components/LevelTVComponent.h"

#include <glm/glm.hpp>
#include <glm/ext/scalar_constants.hpp> // glm::pi
#include <unordered_map>
#include <algorithm>
#include <set>

static std::vector<texture_t> customImages;

static unsigned int maxRange = 0;

bool ReadFile(const std::string& filepath, file_t& file)
{
	FILE* f = NULL;
	fopen_s(&f, filepath.c_str(), "rb");
	if (f)
	{
		fseek(f, 0, SEEK_END);
		file.size = ftell(f);
		fseek(f, 0, SEEK_SET);
		file.data = new file_t::data_t[file.size];
		fread(file.data, file.size, 1, f);
		fclose(f);
		return true;
	}

	file.data = NULL;
	return false;
}

struct levelext_t
{
	addr_t modelAddress;
	u32 nObjects;
	addr_t objAddress;
	u32 nSkybox;
	addr_t skyboxAddress;
	float skyboxRGB[3];
};

struct geo_t
{
	bool isLevel;
	addr_t bspAddress;
	u32 vertexCount;
	u32 polygonCount;
	u32 vertexColorCount;
	u32 boneCount;
	addr_t vertexAddress;
	addr_t polygonAddress;
	addr_t vertexColorAddress;
	addr_t materialAddress;
	addr_t boneAddress;
	addr_t textureAnimAddress;
};

void ReadVertices(file_t& dfx, level_t& level, levelext_t& levelData, geo_t& geo, std::shared_ptr<Model> model)
{
	dfx.seek(geo.vertexAddress);
	for (u32 i = 0; i < geo.vertexCount; ++i)
	{
		model->vertices.push_back(
			{
				dfx.Read<i16>(i * 12 + 0), dfx.Read<i16>(i * 12 + 4), (short)-dfx.Read<i16>(i * 12 + 2),
				dfx.Read<i16>(i * 12 + 0), dfx.Read<i16>(i * 12 + 4), (short)-dfx.Read<i16>(i * 12 + 2),
				dfx.Read<u16>(i * 12 + 6),
				dfx.Read<byte>(i * 12 + 8), dfx.Read<byte>(i * 12 + 9), dfx.Read<byte>(i * 12 + 10), dfx.Read<byte>(i * 12 + 11)
			});

		if (!geo.isLevel)
		{
			model->vertices.back().r = 128;
			model->vertices.back().g = 128;
			model->vertices.back().b = 128;
			model->vertices.back().a = 255;
		}
	}
	dfx.pop();
}

void ReadPolygons(file_t& dfx, level_t& level, levelext_t& levelData, geo_t& geo, std::shared_ptr<Model> model)
{
	dfx.seek(geo.polygonAddress);
	bool hasTexturedFace = geo.isLevel;
	for (u32 i = 0; i < geo.polygonCount; ++i)
	{
		Model::polygon_t polygon;
		const byte stride = geo.isLevel ? 0x14 : 0x0C;
		polygon.vertex[0] = dfx.Read<u16>(stride * i + 0);
		polygon.vertex[1] = dfx.Read<u16>(stride * i + 2);
		polygon.vertex[2] = dfx.Read<u16>(stride * i + 4);
		polygon.flags = dfx.Read<byte>(stride * i + 7);

		if (geo.isLevel)
		{
			addr_t materialAddr = dfx.Read<addr_t>(stride * i + 0x10);

			if (materialAddr != 0xFFFF && (polygon.flags & 0x80) != 0x80)
			{
				dfx.seek(materialAddr);
				polygon.uvs[0] = { dfx.Read<byte>(0) / 255.f, dfx.Read<byte>(1) / 255.f };
				polygon.uvs[1] = { dfx.Read<byte>(4) / 255.f, dfx.Read<byte>(5) / 255.f };
				polygon.uvs[2] = { dfx.Read<byte>(8) / 255.f, dfx.Read<byte>(9) / 255.f };
				polygon.materialID = dfx.Read<u16>(6) % 0x1000;
				dfx.pop();
			}
			else
			{
				polygon.uvs[0].x = polygon.uvs[0].y = 0;
				polygon.uvs[1].x = polygon.uvs[1].y = 0;
				polygon.uvs[2].x = polygon.uvs[2].y = 0;
				polygon.materialID = 0xFFFFFFFF;
				if (materialAddr != 0 && materialAddr < maxRange)
				{
					if (auto info = FindImageInfoById(level.list, ECustomImageType::INFO_EMPTY))
					{
						const float x = (info->x + info->width / 2) / (float)level.sheet.w;
						const float y = (info->y + info->height / 2) / (float)level.sheet.h;
						polygon.uvs[0].x = polygon.uvs[1].x = polygon.uvs[2].x = x;
						polygon.uvs[0].y = polygon.uvs[1].y = polygon.uvs[2].y = y;
					}
					polygon.materialID = ECustomImageType::INFO_EMPTY;
					polygon.isTrigger = true;

					if (!level.signals.contains(materialAddr))
					{
						level.signals[materialAddr] = {};
						ParseCommands(level, dfx, materialAddr, level.signals[materialAddr].commands);
						if (!level.signals[materialAddr].commands.empty())
							level.signals[materialAddr].commands[0] += "#Trigger";
						bool print = false;
						for (auto& c : level.signals[materialAddr].commands)
							if (c.find("Unknown") != std::string::npos)
							{
								print = true;
								break;
							}
						if (print)
						{
							printf("Script? : %x\n", materialAddr);
							for (auto& c : level.signals[materialAddr].commands)
								printf("%s\n", c.c_str());
						}
					}
				}
			}
		}
		else
		{
			if ((polygon.flags & 0x02) == 0x02)
			{
				hasTexturedFace = true;
				addr_t materialAddr = dfx.Read<addr_t>(stride * i + 8);
				dfx.seek(materialAddr);
				//polygon.materialID = dfx.Read<u16>(6) % 0x1000;
				//if (model->name == "charger_" || model->name == "batt____" || model->name == "launch__")
				//{
				//	printf("MAT: %d, FLG: %x\n", polygon.materialID, polygon.flags);
				//}
				
				//polygon.materialID = dfx.Read<u16>(6) % 0x1000;
				//if (geo.textureAnimAddress != 0 && (polygon.flags & 0x8))
				//{
				//	dfx.seek(geo.textureAnimAddress);
				//	dfx.seek(dfx.Read<addr_t>(4), true);
				//	polygon.materialID = dfx.Read<u16>(6) % 0x1000;
				//	//polygon.uvs[0] = { dfx.Read<byte>(0) / 255.f, dfx.Read<byte>(1) / 255.f };
				//	//polygon.uvs[1] = { dfx.Read<byte>(4) / 255.f, dfx.Read<byte>(5) / 255.f };
				//	//polygon.uvs[2] = { dfx.Read<byte>(8) / 255.f, dfx.Read<byte>(9) / 255.f };
				//	dfx.pop();
				//}
				//else
				{
					polygon.materialID = dfx.Read<u16>(6) % 0x1000;
				}

				polygon.uvs[0] = { dfx.Read<byte>(0) / 255.f, dfx.Read<byte>(1) / 255.f };
				polygon.uvs[1] = { dfx.Read<byte>(4) / 255.f, dfx.Read<byte>(5) / 255.f };
				polygon.uvs[2] = { dfx.Read<byte>(8) / 255.f, dfx.Read<byte>(9) / 255.f };
				
				dfx.pop();
			}
			else
			{
				polygon.materialID = 0xFFFFFFFF;
				for (int j = 0; j < 4; ++j)
				{
					polygon.optColors[j] = dfx.Read<byte>(8 + j);
				}
			}
		}
		model->polygons.push_back(polygon);
	}
	model->hasNoTextures = !hasTexturedFace;
	dfx.pop();
}

void ReadSkybox(file_t& dfx, level_t& level, levelext_t& levelData, geo_t& geo, std::shared_ptr<Model> model)
{
	dfx.seek(levelData.skyboxAddress);

	model->name = "@Skybox";
	model->instances.push_back({ {0, 0, 0}, {0, 0, 0} });

	for (u32 i = 0; i < levelData.nSkybox; ++i)
	{
		u32 vertexOffset = model->vertices.size();
		u16 nPoly = dfx.Read<u16>(2, true);
		addr_t vertAddr = dfx.Read<addr_t>(0, true);
		addr_t polyAddr = dfx.Read<addr_t>(0, true);
		u32 nVerts = dfx.Read<u32>(8, true);

		dfx.seek(vertAddr);
		for (u32 v = 0; v < nVerts; ++v)
		{
			int x = (short)(dfx.Read<i16>(0, true)) * 20;
			int y = (short)(dfx.Read<i16>(0, true)) * 20;
			int z = (short)(dfx.Read<i16>(0, true)) * 20;
			u16 n = dfx.Read<u16>(0, true);
			model->vertices.push_back(Model::vertex_t{ x, z, -y, x, z, -y, n, 128, 128, 128, 255 });
		}

		dfx.seek(polyAddr, true);
		for (u32 p = 0; p < nPoly; ++p)
		{
			Model::polygon_t poly;
			poly.vertex[0] = dfx.Read<u16>(0, true) + vertexOffset;
			poly.vertex[1] = dfx.Read<u16>(0, true) + vertexOffset;
			poly.vertex[2] = dfx.Read<u16>(0, true) + vertexOffset;
			poly.flags = dfx.Read<u16>(0, true) + vertexOffset;

			addr_t materialAddress = dfx.Read<addr_t>(0, true);
			u32 tempOffset = dfx.baseOffset;
			dfx.seek(materialAddress);
			poly.uvs[0].x = dfx.Read<byte>(0, true) / 255.f;
			poly.uvs[0].y = dfx.Read<byte>(0, true) / 255.f;
			poly.uvs[1].x = dfx.Read<byte>(2, true) / 255.f;
			poly.uvs[1].y = dfx.Read<byte>(0, true) / 255.f;
			poly.materialID = dfx.Read<u16>(0, true) % 0x1000;
			poly.uvs[2].x = dfx.Read<byte>(0, true) / 255.f;
			poly.uvs[2].y = dfx.Read<byte>(0, true) / 255.f;
			dfx.pop();

			//if (auto info = FindImageInfoById(level.list, poly.materialID))
			//{
			//	for (int j = 0; j < 3; ++j)
			//	{
			//		poly.uvs[j].x *= info->width;
			//		poly.uvs[j].y *= info->height;
			//		poly.uvs[j].x += info->x;
			//		poly.uvs[j].y += info->y;
			//		poly.uvs[j].x /= (float)level.sheet.w;
			//		poly.uvs[j].y /= (float)level.sheet.h;
			//	}
			//}

			model->polygons.push_back(poly);
		}
		dfx.pop();
	}
	dfx.pop();
}

void ReadLevelGeometry(file_t& dfx, level_t& level, levelext_t& levelData, addr_t geometryAddress)
{
	dfx.seek(geometryAddress);
	geo_t geo;
	geo.isLevel = true;
	geo.bspAddress = dfx.Read<addr_t>(0);
	geo.vertexCount = dfx.Read<u32>(0x18);
	geo.polygonCount = dfx.Read<u32>(0x1C);
	geo.vertexColorCount = dfx.Read<u32>(0x20);
	geo.vertexAddress = dfx.Read<addr_t>(0x24);
	geo.polygonAddress = dfx.Read<addr_t>(0x28);
	geo.vertexColorAddress = dfx.Read<addr_t>(0x2C);
	geo.materialAddress = dfx.Read<addr_t>(0x30);
	level.models.push_back(std::make_shared<Model>(0xFFFF'FFFF));
	level.models.back()->name = "@Level";

	ReadVertices(dfx, level, levelData, geo, level.models[0]);
	ReadPolygons(dfx, level, levelData, geo, level.models[0]);

	auto skybox = std::make_shared<Model>(levelData.skyboxAddress);
	ReadSkybox(dfx, level, levelData, geo, skybox);
	level.models.push_back(skybox);

	dfx.pop();
}

void ReadObjectGeometry(file_t& dfx, level_t& level, levelext_t& levelData, addr_t modelAddr)
{
	dfx.seek(modelAddr);
	auto model = std::make_shared<Model>(modelAddr);
	level.models.push_back(model);
	addr_t modelNameAddr = dfx.Read<addr_t>(0x24);
	char name[9] = { 0 };
	memcpy(name, dfx.ptrAt<byte>(modelNameAddr), 8);
	printf("Reading %s model data...\n", name);
	model->name = name;

	if (model->name == "proxsig_")
	{
		CreateSpriteObject(level, model, "proxsig_", ECustomImageType::INFO_PROXSIG, 2);
		dfx.pop();
		return;
	}
	else if (model->name == "cold____")
	{
		CreateSpriteObject(level, model, "cold____", ECustomImageType::INFO_COLD, 1);
		dfx.pop();
		return;
	}

	u16 objCount = dfx.Read<u16>(8);
	addr_t objStartAddr = dfx.Read<u32>(12);

	for (u16 i = 0; i < objCount; ++i)
	{
		dfx.seek(0, true);
		dfx.seek(dfx.Read<addr_t>(objStartAddr + i * 4), true);

		geo_t geo;

		geo.isLevel = false;
		geo.vertexCount = dfx.Read<u16>(0, true);
		geo.vertexAddress = dfx.Read<addr_t>(2, true);
		geo.polygonCount = dfx.Read<u16>(8, true);
		geo.polygonAddress = dfx.Read<addr_t>(2, true);
		geo.boneCount = dfx.Read<u16>(0, true);
		geo.boneAddress = dfx.Read<addr_t>(2, true);
		geo.textureAnimAddress = dfx.Read<addr_t>(0, true);

		ReadVertices(dfx, level, levelData, geo, model);
		ReadPolygons(dfx, level, levelData, geo, model);
	}

	dfx.pop();
}

void ReadObjectInstance(file_t& dfx, level_t& level, levelext_t& levelData, addr_t instanceAddr)
{
	dfx.seek(instanceAddr);
	addr_t modelAddr = dfx.Read<addr_t>(0);
	u32 modelIndex = 0;
	for (auto& m : level.models)
	{
		if (m->addr == modelAddr)
			break;
		++modelIndex;
	}

	//if (modelIndex == level.models.size())
	//{
	//	ReadObjectGeometry(dfx, level, levelData, modelAddr);
	//}
	
	constexpr float c_PI_2_FROM_1024 = glm::pi<float>() / 2048.f;
	glm::vec3 rot = { -dfx.Read<i16>(8) * c_PI_2_FROM_1024, dfx.Read<i16>(12) * -c_PI_2_FROM_1024, dfx.Read<i16>(10) * -c_PI_2_FROM_1024 };
	if (dfx.Read<i16>(14) != 0)
		printf("Oops: %x (%s)\n", dfx.Read<i16>(14), level.models[modelIndex]->name.c_str());
	if (rot.x != 0)
		printf("OopsX: %f (%s)\n", rot.x, level.models[modelIndex]->name.c_str());
	if (rot.z != 0)
		printf("OopsZ: %f (%s)\n", rot.z, level.models[modelIndex]->name.c_str());
	glm::vec3 pos = { -dfx.Read<i16>(16) * 0.001f, -dfx.Read<i16>(20) * 0.001f, dfx.Read<i16>(18) * 0.001f };
	level.models[modelIndex]->instances.push_back({ pos, rot, true, instanceAddr + dfx.baseOffset, dfx.Read<u32>(28),
		{
			dfx.Read<u32>(0x20),
			dfx.Read<u32>(0x24),
			dfx.Read<u32>(0x28),
			dfx.Read<u32>(0x2C),
		}
	});

	dfx.pop();

	ReadComponents(dfx, level, *level.models[modelIndex]);

	if ((level.models[modelIndex]->name == "lvltv___" && level.models[modelIndex]->instances.back().instanceData[2] == 0))
	{
		auto& inst = level.models[modelIndex]->instances.back();
		if (auto it = std::find_if(level.models.begin(), level.models.end(), [](std::shared_ptr<Model> model)
			{
				return model->name == "etvbutn_";
			}); it != level.models.end())
		{
			(*it)->instances.push_back({
				inst.position,
				inst.rotation,
				true,
				inst.address | 0x8000'0000,
				0
			});
		}

		const static std::unordered_map<std::string, int> remData = {
			{ "circuit5", 2 },
			{ "circuit9", 3 },
			{ "horror2", 3 },
			{ "horror4", 3 },
			{ "horror5", 1 },
			{ "kungfu1", 3 },
			{ "kungfu02", 2 },
			{ "looney30", 3 },
			{ "looney69", 2 },
			{ "prehst1", 2 },
			{ "prehst2", 3 },
			{ "prehst3", 1 },
			{ "scifi10", 2 },
			{ "scifi14", 3 },
			{ "rezop1", 2 },
			{ "rezop3", 1 },
			{ "train30", 3 }
		};

		if (auto ptr = inst.GetComponent<LevelTVComponent>())
		{
			const std::string id = ptr->levelType + std::to_string(ptr->levelNum);
			if (auto tvit = remData.find(id); tvit != remData.end())
			{
				if (auto it = std::find_if(level.models.begin(), level.models.end(), [](std::shared_ptr<Model> model)
					{
						return model->name == "remrlow_";
					}); it != level.models.end())
				{
					if (tvit->second == 1 || tvit->second == 3)
						(*it)->instances.push_back({
							inst.position + glm::vec3{0.f, -1.5f, 0.f},
							inst.rotation,
							true,
							inst.address | 0x8000'0000,
							0
							});

					if (tvit->second > 1)
					{
						auto dir = glm::vec3{ cosf(inst.rotation.y) / (4.f - tvit->second), 0, sinf(inst.rotation.y) / (4.f - tvit->second) } / 3.f;
						(*it)->instances.push_back({
							inst.position + glm::vec3{0.f, -1.5f, 0.f} - dir,
							inst.rotation,
							true,
							inst.address | 0x8000'0000,
							0
							});
						(*it)->instances.push_back({
							inst.position + glm::vec3{0.f, -1.5f, 0.f} + dir,
							inst.rotation,
							true,
							inst.address | 0x8000'0000,
							0
							});
					}
				}
			}
		}
	}
	
	if (level.models[modelIndex]->name.find("plaq") != std::string::npos)
	{
		level.models[modelIndex]->instances.back().position += glm::vec3{0, -0.2f, 0};
	}

	if (level.models[modelIndex]->name.starts_with("endtv") && static_cast<EndTVComponent*>(level.models[modelIndex]->instances.back().components[0].get())->condition == 0)
	{
		if (auto it = std::find_if(level.models.begin(), level.models.end(), [](std::shared_ptr<Model> model)
			{
				return model->name == "etvbutn_";
			}); it != level.models.end())
		{
			auto& inst = level.models[modelIndex]->instances.back();
			(*it)->instances.push_back({
				inst.position,
				inst.rotation,
				true,
				inst.address | 0x8000'0000,
				0
				});
		}
	}
}

struct GexTex_t
{
	struct TexInfo_t
	{
		GrLOD_t smallLod;
		GrLOD_t largeLod;
		GrAspectRatio_t aspectRatio;
		GrTextureFormat_t format;
	} info;
	struct NCCTable_t
	{
		FxU8 yRGB[16];
		FxI16 iRGB[4][3];
		FxI16 qRGB[4][3];
		FxU32 packed_data[12];
	} ncctable;
	FxU32 smallLodBytes;
	FxU32 largeLodBytes;
};

auto GetImageSizeFromTexture(GrLOD_t lod, GrAspectRatio_t aspect)
{
	struct size
	{
		u32 w, h;
	};

	const FxU32 magicNum = 256U >> lod;

	switch (aspect)
	{
	case GrAspectRatio_t::GR_ASPECT_8x1:
		return size{ magicNum, magicNum };

	case GrAspectRatio_t::GR_ASPECT_4x1:
		return size{ magicNum, magicNum };

	case GrAspectRatio_t::GR_ASPECT_2x1:
		return size{ magicNum, magicNum };

	case GrAspectRatio_t::GR_ASPECT_1x2:
		return size{ magicNum, magicNum >> 1 };

	case GrAspectRatio_t::GR_ASPECT_1x4:
		return size{ magicNum, magicNum >> 2 };

	case GrAspectRatio_t::GR_ASPECT_1x8:
		return size{ magicNum, magicNum >> 3 };

	default:
	case GrAspectRatio_t::GR_ASPECT_1x1:
		return size{ magicNum, magicNum };
	}
}

glm::vec4* ConvertARGB4444(file_t& vfx, const GexTex_t& tex)
{
	auto [w, h] = GetImageSizeFromTexture(tex.info.largeLod, tex.info.aspectRatio);

	glm::vec4* buffer = new glm::vec4[w * h];

	for (size_t i = 0; i < tex.largeLodBytes / 2; ++i)
	{
		FxU16 pixel_data = vfx.Read<FxU16>(0, true);
#pragma warning(push)
#pragma warning(disable: 6386)
		buffer[i] = {
			(FxU8)(((pixel_data & 0x0F00) >> 8) * 0x11),
			(FxU8)(((pixel_data & 0x00F0) >> 4) * 0x11),
			(FxU8)(((pixel_data & 0x000F)) * 0x11),
			(FxU8)(((pixel_data & 0xF000) >> 12) * 0x11)
		};
#pragma warning(pop)
	}

	return buffer;
}

glm::vec4* ConvertARGB1555(file_t& vfx, const GexTex_t& tex)
{
	auto [w, h] = GetImageSizeFromTexture(tex.info.largeLod, tex.info.aspectRatio);

	glm::vec4* buffer = new glm::vec4[w * h];

	for (size_t i = 0; i < tex.largeLodBytes / 2; ++i)
	{
		FxU16 pixel_data = vfx.Read<FxU16>(0, true);
#pragma warning(push)
#pragma warning(disable: 6386)
		buffer[i] = {
			(FxU8)(((pixel_data >> 10) & 0x1F) * 0x08),
			(FxU8)(((pixel_data >> 5) & 0x1F) * 0x08),
			(FxU8)(((pixel_data) & 0x1F) * 0x8),
			(FxU8)(((pixel_data & 0x8000) ? 0xFF : 0x00))
		};
#pragma warning(pop)
	}

	return buffer;
}

glm::vec4* ConvertYIQ422(file_t& vfx, const GexTex_t& tex)
{
	auto [w, h] = GetImageSizeFromTexture(tex.info.largeLod, tex.info.aspectRatio);

	glm::vec4* buffer = new glm::vec4[w * h];

	GexTex_t::NCCTable_t ncc;
	const GexTex_t::NCCTable_t* ncc1 = &tex.ncctable;
	memcpy(&ncc, ncc1, sizeof(GexTex_t::NCCTable_t));

	for (int i = 0; i < 4; ++i)
	{
		if (ncc.iRGB[i][0] & 0x100)
			ncc.iRGB[i][0] |= 0xff00;
		if (ncc.iRGB[i][1] & 0x100)
			ncc.iRGB[i][1] |= 0xff00;
		if (ncc.iRGB[i][2] & 0x100)
			ncc.iRGB[i][2] |= 0xff00;

		if (ncc.qRGB[i][0] & 0x100)
			ncc.qRGB[i][0] |= 0xff00;
		if (ncc.qRGB[i][1] & 0x100)
			ncc.qRGB[i][1] |= 0xff00;
		if (ncc.qRGB[i][2] & 0x100)
			ncc.qRGB[i][2] |= 0xff00;
	}

	for (size_t i = 0; i < tex.largeLodBytes; ++i)
	{
		FxU8 in = vfx.Read<FxU8>(0, true);

		FxI32 R = ncc.yRGB[in >> 4] + ncc.iRGB[(in >> 2) & 0x3][0]
			+ ncc.qRGB[(in) & 0x3][0];

		FxI32 G = ncc.yRGB[in >> 4] + ncc.iRGB[(in >> 2) & 0x3][1]
			+ ncc.qRGB[(in) & 0x3][1];

		FxI32 B = ncc.yRGB[in >> 4] + ncc.iRGB[(in >> 2) & 0x3][2]
			+ ncc.qRGB[(in) & 0x3][2];

		R = ((R < 0) ? 0 : ((R > 255) ? 255 : R));
		G = ((G < 0) ? 0 : ((G > 255) ? 255 : G));
		B = ((B < 0) ? 0 : ((B > 255) ? 255 : B));

#pragma warning(push)
#pragma warning(disable: 6386)
		buffer[i] = {
			(FxU8)(R), (FxU8)(G), (FxU8)(B), 0xFF
		};
#pragma warning(pop)

		++in;
	}

	return buffer;
}

glm::vec4* ReadTexture(file_t& vfx, const GexTex_t& tex)
{
	switch (tex.info.format)
	{
	case GrTextureFormat_t::GR_TEXFMT_ARGB_4444:
		return ConvertARGB4444(vfx, tex);

	case GrTextureFormat_t::GR_TEXFMT_ARGB_1555:
		return ConvertARGB1555(vfx, tex);

	case GrTextureFormat_t::GR_TEXFMT_YIQ_422:
		return ConvertYIQ422(vfx, tex);

	default:
		printf("Unknown type: %d\n", tex.info.format);
		return NULL;
	}
}

void BlitTex(texture_t& dst, const texture_t& src, int x, int y)
{
	for (u32 yi = 0; yi < src.h; ++yi)
	{
		for (u32 xi = 0; xi < src.w; ++xi)
		{
			dst.pixels[(y + yi) * dst.w + (x + xi)] = src.pixels[yi * src.w + xi];
		}
	}
}

void LoadTextures(const std::string& filepath, level_t& level)
{
	file_t vfx;
	if (!ReadFile(filepath, vfx))
		return;

	u32 numTex = vfx.Read<u32>(0);

	vfx.baseOffset = 4;
	GexTex_t gexTex;
	for (u32 i = 0; i < numTex; ++i)
	{
		gexTex.info.smallLod = vfx.Read<GrLOD_t>(0, true);
		gexTex.info.largeLod = vfx.Read<GrLOD_t>(0, true);
		gexTex.info.aspectRatio = vfx.Read<GrAspectRatio_t>(0, true);
		gexTex.info.format = vfx.Read<GrTextureFormat_t>(0, true);
		(void)vfx.Read<FxU32>(0, true); // addr

		for (int i = 0; i < 16; ++i)
			gexTex.ncctable.yRGB[i] = vfx.Read<FxU8>(0, true);

		for (int y = 0; y < 4; ++y)
			for (int x = 0; x < 3; ++x)
				gexTex.ncctable.iRGB[y][x] = vfx.Read<FxI16>(0, true);

		for (int y = 0; y < 4; ++y)
			for (int x = 0; x < 3; ++x)
				gexTex.ncctable.qRGB[y][x] = vfx.Read<FxI16>(0, true);

		for (int i = 0; i < 12; ++i)
		{
			gexTex.ncctable.packed_data[i] = vfx.Read<FxU32>(0, true);
		}

		gexTex.smallLodBytes = vfx.Read<FxU32>(0, true);
		gexTex.largeLodBytes = vfx.Read<FxU32>(0, true);

		auto t = ReadTexture(vfx, gexTex);
		auto [w, h] = GetImageSizeFromTexture(gexTex.info.largeLod, gexTex.info.aspectRatio);
		if (t != NULL)
		{
			for (u32 ii = 0; ii < w * h; ++ii)
			{
				auto& pixel = t[ii];
				pixel.r /= 255.f;
				pixel.g /= 255.f;
				pixel.b /= 255.f;
				pixel.a /= 255.f;
			}
		}
		if (auto info = FindImageInfoById(level.list, i))
		{
			BlitTex(level.sheet, texture_t{ w, h, t }, info->x, info->y);
		}
		//delete[] t;
		level.textures.push_back(texture_t{ w, h, t, true, gexTex.info.format == GrTextureFormat_t::GR_TEXFMT_ARGB_1555 });
	}
	
	for(size_t i = 0; i < customImages.size(); ++i)
	{
		if (auto info = FindImageInfoById(level.list, ECustomImageType::CUSTOM_IMAGE_BASE + i))
		{
			BlitTex(level.sheet, customImages[i], info->x, info->y);
		}
		level.textures.push_back(customImages[i]);
	}
}

void LoadCustomImages()
{
	if (!customImages.empty())
		return;

	file_t file;
	std::string rootType = "..";
	if (!ReadFile("../data/images/spawn.png", file))
	{
		rootType = ".";
		if (!ReadFile("./data/images/spawn.png", file))
			return; // Failed to open both files
	}

	file.Close();

	const std::vector<std::string> filesToLoad = {
		"/data/images/spawn.bin",
		"/data/images/proxsig.bin",
		"/data/images/unknown.bin",
		"/data/images/point.bin",
		"/data/images/cold.bin",
		"/data/images/white.bin"
	};

	for (auto& fileName : filesToLoad)
	{
		if (ReadFile(rootType + fileName, file))
		{
			texture_t image;
			image.w = file.Read<u32>(0);
			image.h = file.Read<u32>(4);

			const unsigned short nPalette = file.Read<u16>(8);
			std::vector<glm::vec4> palette;
			for (int i = 0; i < nPalette; ++i)
				palette.push_back({
					file.Read<byte>(i * 4 + 10 + 3) / 255.f,
					file.Read<byte>(i * 4 + 10 + 2) / 255.f,
					file.Read<byte>(i * 4 + 10 + 1) / 255.f,
					file.Read<byte>(i * 4 + 10 + 0) / 255.f,
				});

			const byte compression = file.Read<byte>(10 + 4 * nPalette);
			enum CompressionFormat
			{
				NONE = 0,
				RLE = 1
			};

			image.pixels = new glm::vec4[image.w * image.h];
			image.deletePixels = false;

			switch (compression)
			{
			case NONE:
				for (u32 y = 0; y < image.h; ++y)
					for (u32 x = 0; x < image.w; ++x)
					{
						size_t index = y * image.w + x;
						image.pixels[index] = palette[file.Read<byte>(10 + 4 * nPalette + 1 + index)];
					}
				break;

			case RLE:
			{
				const u32 len = file.Read<u32>(10 + 4 * nPalette + 1, true);
				u32 index = 0;
				int count = 0;
				byte pid = 0;
				for(u32 entryIndex = 0; entryIndex < len / 2; ++entryIndex)
				{
					count = file.Read<byte>(0);
					pid = file.Read<byte>(1, true);
					for (int i = 0; i < count; ++i)
					{
#pragma warning(suppress: 6386) // No real buffer overrun
						image.pixels[index++] = palette[pid];
					}
				}
				break;
			}
			}

			customImages.push_back(image);
			file.Close();
		}
	}
}

bool GetTextureInformation(const std::string& filepath, ImagePacker::ImageInformationList& list)
{
	file_t f;
	if (ReadFile(filepath, f))
	{
		u32 nFiles = f.Read<u32>(0, true);
		for (u32 i = 0; i < nFiles; ++i)
		{
			GrLOD_t lod = f.Read<GrLOD_t>(sizeof(GrLOD_t), true);
			GrAspectRatio_t asp = f.Read<GrAspectRatio_t>(0, true);
			// Skip data
			(void)f.Read<u32>(f.Read<u32>(0x7C, true) - 4, true);

			auto [w, h] = GetImageSizeFromTexture(lod, asp);
			list.push_back({ (int)w, (int)h, (void*)i });
		}
		LoadCustomImages();
		for(size_t i = 0; i < customImages.size(); ++i)
			list.push_back({ (int)customImages[i].w, (int)customImages[i].h, (void*)(ECustomImageType::CUSTOM_IMAGE_BASE + i)});
		return true;
	}
	return false;
}

std::string GetLevelName(const std::string& levelStr, u32 dataOffsetRaw)
{
	const std::unordered_map<std::string, std::string> QuickMap = {
		{ "spy_____", "The Spy Who Loved Himself" },
		{ "nypd____", "In Drag Net" },
		{ "gillig__", "Gilligex Isle" },
		{ "mooshu__", "Mooshu Pork" },
		{ "gexzil__", "Gexzilla Vs. Mecharez" },
		{ "final___", "Channel Z" },
		{ "train___", "Poltergex" },
		{ "junk____", "I Got the Reruns" },
		{ "aztec___", "Aztec 2 Step" },
		{ "lost____", "Trouble in Uranus" }
	};

	if (auto it = QuickMap.find(levelStr); it != QuickMap.end())
		return it->second;

	struct LevelInfo_t
	{
		std::string name;
		u32 value;
	};

	const std::unordered_map<std::string, std::unordered_map<u32, std::string>> QuickMap2 = {
		{ "looney__",
			{
				{ 0x00006208, "Out of Toon" },
				{ 0x0000204E, "OoT67" },
				{ 0x00005C2D, "Fine Tooning" },
				//{ 0x00000000, "OoT70" } // Can't actually be loaded, no data
				{ 0x0000275A, "OoT88" }
			}
		},
		{ "circuit_",
			{
				{ 0x00002411, "Chips and Dips" },
				{ 0x0000694F, "www.dotcom.com" },
				{ 0x00007242, "Honey I Shrunk the Gecko" },
			}
		},
		{ "horror__",
			{
				{ 0x00006BE3, "Frankensteinfeld" },
				{ 0x0000737B, "Smellraiser" },
				{ 0x000051F3, "Texas Chainsaw Manicure" },
				{ 0x00002B27, "Thursday the 12th" }
			}
		},
		{ "kungfu__",
			{
				{ 0x00006FC1, "Samurai Night Fever" },
				{ 0x00006C72, "Mao Tse Tongue" },
				{ 0x00002F95, "Lizard in a China Shop" }
			}
		},
		{ "scifi___",
			{
				{ 0x00005501, "The Umpire Strikes Out" },
				{ 0x0000469E, "Pain in the Asteroids" }
			}
		},
		{ "rezop___",
			{
				{ 0x00006A05, "Mazed and Confused" },
				{ 0x00000BBD, "Bugged Out" },
				{ 0x00007552, "No Weddings and a Funeral" }
			}
		},
		{ "prehst__",
			{
				{ 0x000045B0, "Pangaea 90210" },
				{ 0x000040DE, "This Old Cave" },
				{ 0x00005DB3, "Lava Daba Doo" }
			}
		},
		{ "map_____",
			{
				{ 0x000071A0, "The Media Dimension" },
				{ 0x00006414, "Main Menu" },
				{ 0x00004D0A, "Credits Menu" }
			}
		}
	};

	if (auto it1 = QuickMap2.find(levelStr); it1 != QuickMap2.end())
	{
		if (auto it2 = it1->second.find(dataOffsetRaw); it2 != it1->second.end())
		{
			return it2->second;
		}
	}

	return "Unknown Level";
}

bool LoadLevel(const std::string& filepath, level_t& level)
{
	file_t dfx;
	if (!ReadFile(filepath, dfx))
		return false;

	levelext_t levelData;

	const std::string vfxPath = filepath.substr(0, filepath.find_last_of(".")) + ".vfx";
	if (level.sheet.pixels)
	{
		delete[] level.sheet.pixels;
		level.sheet.pixels = NULL;
	}
	level.list.clear();
	if (GetTextureInformation(vfxPath, level.list))
	{
		if (int size = ImagePacker::GeneratePackedList(level.list, 256); size != 0)
		{
			printf("Sheet generated at %dx%d\n", size, size);
			level.sheet = { (unsigned int)size, (unsigned int)size, new glm::vec4[size * size] };
			if (level.sheet.pixels)
			{
				for (int y = 0; y < size; ++y)
				{
					for (int x = 0; x < size; ++x)
					{
						if (((x % 128) == (x % 64) && (y % 128) == (y % 64)) || ((x % 128) != (x % 64) && (y % 128) != (y % 64)))
							level.sheet.pixels[x + y * size] = { 1, 0, 1, 1 };
						else
							level.sheet.pixels[x + y * size] = { 0.5, 0, 0.5, 1 };
					}
				}
				LoadTextures(vfxPath, level);
			}
		}
	}

	dfx.baseOffset = level.baseData = ((dfx.Read<u32>(0) + 0x200) >> 9) << 11;

	maxRange = dfx.ReadAt<addr_t>(0x88);
	
	levelData.modelAddress = dfx.Read<addr_t>(0x3C);
	levelData.nObjects = dfx.Read<u32>(0x78);
	levelData.objAddress = dfx.Read<addr_t>(0x7C);
	levelData.nSkybox = dfx.Read<u32>(0x20);
	levelData.skyboxAddress = dfx.Read<u32>(0x24);
	level.bgColor[0] = dfx.Read<byte>(68) / 255.f;
	level.bgColor[1] = dfx.Read<byte>(69) / 255.f;
	level.bgColor[2] = dfx.Read<byte>(70) / 255.f;

	ReadLevelGeometry(dfx, level, levelData, dfx.Read<addr_t>(0));

	std::shared_ptr<Model> misc = std::make_shared<Model>(0);
	CreateSpriteObject(level, misc, "@CameraTarget", ECustomImageType::INFO_UNKNOWN, 1);
	level.models.push_back(misc);

	// Create spawn point
	std::shared_ptr<Model> spawn = std::make_shared<Model>(dfx.baseOffset + 0x28);
	CreateSpriteObject(level, spawn, "$Spawn", ECustomImageType::INFO_SPAWN);
	level.models.push_back(spawn);
	spawn->instances.push_back({});
	spawn->instances[0].position = { dfx.Read<i16>(0x28) * -0.001f, dfx.Read<i16>(0x2C) * -0.001f, dfx.Read<i16>(0x2A) * 0.001f };

	size_t currModelIndex = level.models.size();

	dfx.seek(levelData.modelAddress);
	while (dfx.Read<addr_t>(0) != levelData.modelAddress)
	{
		ReadObjectGeometry(dfx, level, levelData, dfx.Read<addr_t>(0, true));
	}
	dfx.pop();

	for (u32 i = 0; i < levelData.nObjects; ++i)
	{
		ReadObjectInstance(dfx, level, levelData, levelData.objAddress + 0x30 * i);
	}

	// By treating the level as a model, we need to give it an instance
	level.models[0]->instances.push_back({});

	dfx.seek(0, true);
	std::string s;
	s.resize(8);
	memcpy(s.data(), dfx.ptrAt<char>(0xE0), 8);
	{
		dfx.baseOffset = 0;
		level.name = GetLevelName(s, dfx.ReadAt<u32>(0));
		dfx.baseOffset = level.baseData;
	}

	ApplyPathModels(level);

	std::sort(level.models.begin() + currModelIndex, level.models.end(), [](std::shared_ptr<Model> a, std::shared_ptr<Model> b)
		{
			return a->name < b->name;
		});

	memcpy(level.pickupName[0], dfx.ptrAt<char>(0xEC), 8);
	memcpy(level.pickupName[1], dfx.ptrAt<char>(0xF8), 8);
	memcpy(level.pickupName[2], dfx.ptrAt<char>(0x104), 8);
	level.pickupName[0][8] = level.pickupName[1][8] = level.pickupName[2][8] = '\0';

	// Apply object UVs
	for(auto& mdl : level.models)
		for(auto& poly : mdl->polygons)
			if (auto info = FindImageInfoById(level.list, poly.materialID))
			{
				for (int j = 0; j < 3; ++j)
				{
					poly.uvs[j].x *= info->width;
					poly.uvs[j].y *= info->height;
					poly.uvs[j].x += info->x;
					poly.uvs[j].y += info->y;
					poly.uvs[j].x /= (float)level.sheet.w;
					poly.uvs[j].y /= (float)level.sheet.h;
				}
			}

	// Read level scripts
	if (auto& comp = level.models[0]->instances[0].AddComponent<ScriptComponent>(); true)
	{
		ParseCommands(level, dfx, dfx.ReadAt<addr_t>(0x74), comp.commands);
	}
	
	for (int i = 0; i < 6; ++i)
	{
		level.signals[0xB0 + i * 4] = {};
		ParseCommands(level, dfx, dfx.ReadAt<addr_t>(0xB0 + i * 4), level.signals[0xB0 + i * 4].commands);
		if (!level.signals[0xB0 + i * 4].commands.empty())
			level.signals[0xB0 + i * 4].commands[0] += ((i < 3) ? "#UnlockTV" : "#Hint");
	}
	
	for (auto it = level.signals.begin(); it != level.signals.end();)
	{
		if (it->second.commands.empty())
			it = level.signals.erase(it);
		else
			++it;
	}

	for(auto& m : level.models)
		for (auto& i : m->instances)
		{
			i.oposition = i.position;
			i.orotation = i.rotation;
		}

	return true;
}