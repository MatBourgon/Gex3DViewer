#include "mapreader.h"
#include "glideconstants.h"
#include <bit>

struct file_t
{
	using data_t = unsigned char;
	data_t* data;
	size_t size;
	unsigned int baseOffset = 0;

	template<typename T>
	T Read(size_t offset, bool moveOffset = false)
	{
		T data = _Read<T>(baseOffset + offset);
		if constexpr (std::endian::native == std::endian::big && sizeof(T) > 1)
		{
			using byte = unsigned char;
			for (int i = 0; i < sizeof(T) >> 1; ++i)
			{
				static_cast<byte*>(&data)[i] ^= static_cast<byte*>(&data)[sizeof(T) - i - 1];
				static_cast<byte*>(&data)[sizeof(T) - i - 1] ^= static_cast<byte*>(&data)[i];
				static_cast<byte*>(&data)[i] ^= static_cast<byte*>(&data)[sizeof(T) - i - 1];
			}
		}
		if (moveOffset)
			baseOffset += sizeof(T);
		return data;
	}

	~file_t()
	{
		delete[] data;
		size = 0;
	}
	
private:


	template<typename T>
	T _Read(size_t offset)
	{
#ifdef DEBUG
		if (offset >= size)
		{
			printf("READ OUT OF BOUNDS! %x >= %x!\n", offset, size);
		}
#endif
		return *(T*)(data + offset);
	}
};

bool ReadFile(const std::string& filepath, file_t& file)
{
	if (FILE* f = fopen(filepath.c_str(), "rb"))
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

using byte = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using i16 = signed short;
using i32 = signed int;
using addr_t = u32;

struct levelext_t
{
	u32 dataOffset;
	addr_t modelAddress;
};

struct levelgeo_t
{
	addr_t bspAddress;
	u32 vertexCount;
	u32 polygonCount;
	u32 vertexColorCount;
	addr_t vertexAddress;
	addr_t polygonAddress;
	addr_t vertexColorAddress;
	addr_t materialAddress;
};

void ReadLevelGeometry(file_t& dfx, level_t& level, levelext_t& levelData, addr_t geometryAddress)
{
	levelgeo_t geo;
	dfx.baseOffset += geometryAddress;
	geo.bspAddress = dfx.Read<addr_t>(0);
	geo.vertexCount = dfx.Read<u32>(0x18);
	geo.polygonCount = dfx.Read<u32>(0x1C);
	geo.vertexColorCount = dfx.Read<u32>(0x20);
	geo.vertexAddress = dfx.Read<addr_t>(0x24);
	geo.polygonAddress = dfx.Read<addr_t>(0x28);
	geo.vertexColorAddress = dfx.Read<addr_t>(0x2C);
	geo.materialAddress = dfx.Read<addr_t>(0x30);
	level.models.push_back(std::make_shared<Model>());

#pragma region ReadVertices
	dfx.baseOffset = levelData.dataOffset + geo.vertexAddress;
	for (u32 i = 0; i < geo.vertexCount; ++i)
	{
		level.models[0]->vertices.push_back(
			{
				dfx.Read<i16>(i * 12 + 0), dfx.Read<i16>(i * 12 + 4), (short)-dfx.Read<i16>(i * 12 + 2),
				dfx.Read<u16>(i * 12 + 6),
				dfx.Read<byte>(i * 12 + 8), dfx.Read<byte>(i * 12 + 9), dfx.Read<byte>(i * 12 + 10), dfx.Read<byte>(i * 12 + 11)
			});
	}
#pragma endregion

#pragma region ReadPolygons
	dfx.baseOffset = levelData.dataOffset + geo.polygonAddress;
	for (u32 i = 0; i < geo.polygonCount; ++i)
	{
		Model::polygon_t polygon;
		polygon.vertex[0] = dfx.Read<u16>(0x14 * i + 0);
		polygon.vertex[1] = dfx.Read<u16>(0x14 * i + 2);
		polygon.vertex[2] = dfx.Read<u16>(0x14 * i + 4);
		polygon.flags = dfx.Read<u16>(0x14 * i + 6);
		addr_t materialAddr = dfx.Read<addr_t>(0x14 * i + 0x10);
		if (materialAddr != 0xFFFF && (polygon.flags & 0x8000) != 0x8000)
		{
			auto currOffset = dfx.baseOffset;
			dfx.baseOffset = levelData.dataOffset + materialAddr;
			polygon.uvs[0] = { dfx.Read<byte>(0) / 255.f, dfx.Read<byte>(1) / 255.f };
			polygon.uvs[1] = { dfx.Read<byte>(4) / 255.f, dfx.Read<byte>(5) / 255.f };
			polygon.uvs[2] = { dfx.Read<byte>(8) / 255.f, dfx.Read<byte>(9) / 255.f };
			polygon.materialID = dfx.Read<u16>(6);
			dfx.baseOffset = currOffset;
		}
		else
		{
			polygon.materialID = 0xFFFFFFFF;
		}
		//if (materialAddr == 0xFFFF)
		//	polygon.flags |= 0x8000;
		//level.models[0]->vertices.push_back(
		//	{
		//		dfx.Read<i16>(i * 12 + 0), dfx.Read<i16>(i * 12 + 2), dfx.Read<i16>(i * 12 + 4),
		//		dfx.Read<u16>(i * 12 + 6),
		//		dfx.Read<byte>(i * 12 + 8), dfx.Read<byte>(i * 12 + 9), dfx.Read<byte>(i * 12 + 10), dfx.Read<byte>(i * 12 + 11)
		//	});
		level.models[0]->polygons.push_back(polygon);
	}
#pragma endregion

	dfx.baseOffset = levelData.dataOffset;
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

auto GetImageSizeFromTexture(const GexTex_t& texture)
{
	struct size
	{
		u32 w, h;
	};

	const FxU32 magicNum = 256U >> texture.info.largeLod;

	switch (texture.info.aspectRatio)
	{
	case GrAspectRatio_t::GR_ASPECT_8x1:
		return size{ magicNum, magicNum >> 3 };

	case GrAspectRatio_t::GR_ASPECT_4x1:
		return size{ magicNum, magicNum >> 2 };

	case GrAspectRatio_t::GR_ASPECT_2x1:
		return size{ magicNum, magicNum >> 1 };

	case GrAspectRatio_t::GR_ASPECT_1x2:
		return size{ magicNum >> 1, magicNum };

	case GrAspectRatio_t::GR_ASPECT_1x4:
		return size{ magicNum >> 2, magicNum };

	case GrAspectRatio_t::GR_ASPECT_1x8:
		return size{ magicNum >> 3, magicNum };

	default:
	case GrAspectRatio_t::GR_ASPECT_1x1:
		return size{ magicNum, magicNum };
	}
}

glm::vec4* ConvertARGB4444(file_t& vfx, const GexTex_t& tex)
{
	auto [w, h] = GetImageSizeFromTexture(tex);

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
	auto [w, h] = GetImageSizeFromTexture(tex);

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
	auto [w, h] = GetImageSizeFromTexture(tex);

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

		FxI32 R = (FxI32)ncc.yRGB[in >> 4] + ncc.iRGB[(in >> 2) & 0x3][0]
			+ ncc.qRGB[(in) & 0x3][0];

		FxI32 G = (FxI32)ncc.yRGB[in >> 4] + ncc.iRGB[(in >> 2) & 0x3][1]
			+ ncc.qRGB[(in) & 0x3][1];

		FxI32 B = (FxI32)ncc.yRGB[in >> 4] + ncc.iRGB[(in >> 2) & 0x3][2]
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
			gexTex.ncctable.packed_data[i] = vfx.Read<FxU32>(0, true);

		gexTex.smallLodBytes = vfx.Read<FxU32>(0, true);
		gexTex.largeLodBytes = vfx.Read<FxU32>(0, true);

		auto t = ReadTexture(vfx, gexTex);
		auto [w, h] = GetImageSizeFromTexture(gexTex);
		if (t != NULL)
		{
			for (u32 i = 0; i < w * h; ++i)
			{
				auto& pixel = t[i];
				pixel.r /= 255.f;
				pixel.g /= 255.f;
				pixel.b /= 255.f;
				pixel.a /= 255.f;
			}
		}
		level.textures.push_back(texture_t{ w, h, t });
	}
}

bool LoadLevel(const std::string& filepath, level_t& level)
{
	file_t dfx;
	if (!ReadFile(filepath, dfx))
		return false;

	levelext_t levelData;

	levelData.dataOffset = ((dfx.Read<u32>(0) + 0x200) >> 9) << 11;
	dfx.baseOffset = levelData.dataOffset;
	
	levelData.modelAddress = dfx.Read<addr_t>(0x3C);

	ReadLevelGeometry(dfx, level, levelData, dfx.Read<addr_t>(0));

	LoadTextures(filepath.substr(0, filepath.find_last_of(".")) + ".vfx", level);

	return true;
}
