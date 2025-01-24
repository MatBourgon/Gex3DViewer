#pragma once

#include <bit>
#include <vector>

using byte = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using i16 = signed short;
using i32 = signed int;
using addr_t = u32;

struct file_t
{
	using data_t = unsigned char;
	data_t* data;
	size_t size;
	size_t baseOffset = 0;

	template<typename T>
	T Read(size_t offset, bool moveOffset = false)
	{
		T data = _Read<T>(baseOffset + offset + _getoffset());
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
			offsets.back() += sizeof(T) + offset;

		return data;
	}

	// Like Read, but ignores local offset, and thus also can't move the offset
	// For whenever you only really need to read one value from base
	template<typename T>
	T ReadAt(size_t offset)
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

		return data;
	}

	void seek(size_t offset, bool replace = false)
	{
		if (replace)
		{
			if (offsets.empty())
				offsets.push_back(0);
			offsets.back() = offset;
		}
		else
			offsets.push_back(offset);
	}

	void pop()
	{
		if (!offsets.empty())
			offsets.pop_back();
	}

	template<typename T = data_t>
	T* ptr() { return (T*)(data + baseOffset + _getoffset()); }

	// like ptr, but without a local offset
	template<typename T = data_t>
	T* ptrAt(size_t offset) { return (T*)(data + baseOffset + offset); }

	void Close()
	{
		delete[] data;
		data = nullptr;
		size = 0;
		baseOffset = 0;
		offsets.clear();
	}

	~file_t()
	{
		Close();
	}

	size_t _getoffset()
	{
		if (offsets.empty())
			offsets.push_back(0);

		return offsets.back();
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

	std::vector<size_t> offsets;
};