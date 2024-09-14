#pragma once
#include <cmath>

typedef unsigned char FxU8;
typedef signed char FxI8;
typedef unsigned short FxU16;
typedef signed short FxI16;
typedef unsigned int FxU32;
typedef signed int FxI32;

enum GrAspectRatio_t : unsigned int
{
	GR_ASPECT_8x1 = 0x0,       /* 8W x 1H */
	GR_ASPECT_4x1 = 0x1,       /* 4W x 1H */
	GR_ASPECT_2x1 = 0x2,       /* 2W x 1H */
	GR_ASPECT_1x1 = 0x3,       /* 1W x 1H */
	GR_ASPECT_1x2 = 0x4,       /* 1W x 2H */
	GR_ASPECT_1x4 = 0x5,       /* 1W x 4H */
	GR_ASPECT_1x8 = 0x6       /* 1W x 8H */
};

inline GrAspectRatio_t TextureSizeToAspectRatio(int w, int h)
{
	if (w * 8 == h)
		return GR_ASPECT_1x8;
	if (w * 4 == h)
		return GR_ASPECT_1x4;
	if (w * 2 == h)
		return GR_ASPECT_1x2;
	if (w == h * 2)
		return GR_ASPECT_2x1;
	if (w == h * 4)
		return GR_ASPECT_4x1;
	if (w == h * 8)
		return GR_ASPECT_8x1;

	return GR_ASPECT_1x1;
}

enum GrLOD_t : unsigned int
{
	GR_LOD_256 = 0x0,
	GR_LOD_128 = 0x1,
	GR_LOD_64 = 0x2,
	GR_LOD_32 = 0x3,
	GR_LOD_16 = 0x4,
	GR_LOD_8 = 0x5,
	GR_LOD_4 = 0x6,
	GR_LOD_2 = 0x7,
	GR_LOD_1 = 0x8
};

inline GrLOD_t GetLodFromTextureSize(int w, int h)
{
	int s = w < h ? h : w;
	return (GrLOD_t)round(log2(256.f / s));
}

enum GrTextureFormat_t : unsigned int
{
	GR_TEXFMT_8BIT = 0x0,
	GR_TEXFMT_RGB_332 = GR_TEXFMT_8BIT,
	GR_TEXFMT_YIQ_422 = 0x1,
	GR_TEXFMT_ALPHA_8 = 0x2, /* (0..0xFF) alpha     */
	GR_TEXFMT_INTENSITY_8 = 0x3, /* (0..0xFF) intensity */
	GR_TEXFMT_ALPHA_INTENSITY_44 = 0x4,
	GR_TEXFMT_P_8 = 0x5, /* 8-bit palette */
	GR_TEXFMT_RSVD0 = 0x6,
	GR_TEXFMT_RSVD1 = 0x7,
	GR_TEXFMT_16BIT = 0x8,
	GR_TEXFMT_ARGB_8332 = GR_TEXFMT_16BIT,
	GR_TEXFMT_AYIQ_8422 = 0x9,
	GR_TEXFMT_RGB_565 = 0xa,
	GR_TEXFMT_ARGB_1555 = 0xb,
	GR_TEXFMT_ARGB_4444 = 0xc,
	GR_TEXFMT_ALPHA_INTENSITY_88 = 0xd,
	GR_TEXFMT_AP_88 = 0xe, /* 8-bit alpha 8-bit palette */
	GR_TEXFMT_RSVD2 = 0xf
};


enum GrTexTable_t : unsigned int
{
	GR_TEXTABLE_NCC0 = 0x0,
	GR_TEXTABLE_NCC1 = 0x1,
	GR_TEXTABLE_PALETTE = 0x2
};