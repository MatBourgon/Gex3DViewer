#include "imagepacker.h"
#include <algorithm>

constexpr int c_MAXIMAGESIZE = 4096;

class AtlasTree
{
public:
	AtlasTree(int height, ImagePacker::ImageInformation_t* info, int x) : info(info), x(x), height(height), height_used(0) {}
	AtlasTree(int height) : AtlasTree(height, nullptr, 0) {}
	~AtlasTree()
	{
		for(auto& node : nodes)
			delete node;
	}

	std::vector<AtlasTree*> nodes;
	ImagePacker::ImageInformation_t* info;
	int x;
	int height;
	int height_used;

	bool AddNode(ImagePacker::ImageInformation_t* info, const int& max_width)
	{
		const int current_offset = x + (this->info ? this->info->width : 0);

		for (auto& node : nodes)
		{
			if (node->AddNode(info, max_width)) return true;

			if ((info->height) >= height_used && (info->height) <= height && TestNewWidth(info->width, max_width))
			{
				AtlasTree* new_node = new AtlasTree(info->height, info, node->x);
				new_node->nodes.swap(nodes);
				nodes.push_back(new_node);
				for (auto& _node : new_node->nodes)
				{
					_node->AddOffset(info->width);
				}

				new_node->height_used = height_used;
				height_used = new_node->height;

				return true;
			}
		}

		if ((height - height_used) >= (info->height) && (current_offset + info->width) <= max_width)
		{
			nodes.push_back(new AtlasTree(info->height, info, current_offset));
			height_used += info->height;
			return true;
		}

		return false;
	}

	void AddOffset(int w)
	{
		x += w;
		for (auto& node : nodes)
		{
			node->AddOffset(w);
		}
	}

	bool TestNewWidth(int w, const int& max_width)
	{
		if (!info || (x + w + info->width) < max_width)
		{
			for (auto& node : nodes)
			{
				if (!node->TestNewWidth(w, max_width)) return false;
			}
			return true;
		}

		return false;
	}

	void Export(int y)
	{
		if (info)
		{
			info->x = x;
			info->y = y;
		}

		for(auto& node : nodes)
		{
			node->Export(y);
			y += node->height;
		}
	}
};

bool CreateAtlas(ImagePacker::ImageInformationList& list, int size)
{
	AtlasTree tree(size);

	for (auto& info : list)
	{
		if (!tree.AddNode(&info, size))
		{
			return false;
		}
	}

	tree.Export(0);
	return true;
}

int ImagePacker::GeneratePackedList(ImageInformationList& list, int imageStartSizeHint)
{
	int size = imageStartSizeHint;

	// Very bare-bones selection sort, ordered on size
	for (int i = 0; i < list.size(); ++i)
	{
		int bestidx = i;
		for (int j = i; j < list.size(); ++j)
		{
			if (list[j].height > list[bestidx].height)
			{
				bestidx = j;
			}
		}
		if (bestidx != i)
		{
			std::iter_swap(list.begin() + bestidx, list.begin() + i);
		}
	}

	while (size <= c_MAXIMAGESIZE)
	{
		if (CreateAtlas(list, size))
			return size;

		size <<= 1;
	}

	return 0;
}

int ImagePacker::GeneratePackedList(ImageInformationList& list)
{
	return GeneratePackedList(list, 64);
}