#include "cmesh.h"

#include <glm/glm.hpp>
#include <glm/ext/scalar_constants.hpp>

std::shared_ptr<Model> CreatePathPointObject(level_t& level, addr_t addr)
{
	std::shared_ptr<Model> pathPoint = std::make_shared<Model>(addr);
	CreateSpriteObject(level, pathPoint, "$PathPoint", ECustomImageType::INFO_POINT, 1);
	level.models.push_back(pathPoint);
	return pathPoint;
}

void CreateCircleMesh(std::shared_ptr<Model> model, float r, glm::u8vec3 color, int height, int p)
{
	size_t s = model->vertices.size();
	for (int i = 0; i < p + 1; ++i)
	{
		int x = (int)(r * cos(2 * glm::pi<float>() * (i / (float)p)));
		int z = (int)(r * sin(2 * glm::pi<float>() * (i / (float)p)));
		model->vertices.push_back({ x, height, z, x, height, z, 0, color.r, color.g, color.b, 255 });
	}
	model->vertices.push_back({ 0, height, 0, 0, height, 0, 0, color.r, color.g, color.b, 255 });

	for (int i = 0; i < p; ++i)
	{
		model->polygons.push_back({ {s + i, (size_t)p + s, ((i + 1) % p) + s}, 0, 0 });
	}
}

void AddDiamondToModel(std::shared_ptr<Model> model, glm::vec3 pos, float scale)
{
	Model::vertex_t v;
	v.r = v.a = 255;
	v.g = v.b = 0;

	auto addPoint = [&v, model](const glm::vec3& p)
		{
			v.x = v.oX = (int)p.x;
			v.y = v.oY = (int)p.y;
			v.z = v.oZ = (int)p.z;
			model->vertices.push_back(v);
		};

	addPoint(pos + glm::vec3{ scale, 0, 0 });
	addPoint(pos + glm::vec3{ -scale, 0, 0 });
	addPoint(pos + glm::vec3{ 0,  scale, 0 });
	addPoint(pos + glm::vec3{ 0, -scale, 0 });
	addPoint(pos + glm::vec3{ 0, 0,  scale });
	addPoint(pos + glm::vec3{ 0, 0, -scale });

	model->polygons.push_back({
		model->vertices.size() - 2,
		model->vertices.size() - 6,
		model->vertices.size() - 4,
		});

	model->polygons.push_back({
		model->vertices.size() - 6,
		model->vertices.size() - 1,
		model->vertices.size() - 4,
		});

	model->polygons.push_back({
		model->vertices.size() - 1,
		model->vertices.size() - 5,
		model->vertices.size() - 4,
		});

	model->polygons.push_back({
		model->vertices.size() - 5,
		model->vertices.size() - 2,
		model->vertices.size() - 4,
		});

	// ---

	model->polygons.push_back({
		model->vertices.size() - 2,
		model->vertices.size() - 3,
		model->vertices.size() - 6,
		});

	model->polygons.push_back({
		model->vertices.size() - 6,
		model->vertices.size() - 3,
		model->vertices.size() - 1,
		});

	model->polygons.push_back({
		model->vertices.size() - 1,
		model->vertices.size() - 3,
		model->vertices.size() - 5,
		});

	model->polygons.push_back({
		model->vertices.size() - 5,
		model->vertices.size() - 3,
		model->vertices.size() - 2,
		});
}

void AddLineToModel(std::shared_ptr<Model> model, glm::vec3 start, glm::vec3 end)
{
	const glm::vec3 direction = glm::normalize(end - start);
	if (direction.x == 0 && direction.y == 0 && direction.z == 0 || isnan(direction.x) || isnan(direction.y) || isnan(direction.z))
		return; // 0-vector or invalid

	glm::vec3 normal = glm::normalize(glm::cross(direction, { 0, 1, 0 }));
	if (isnan(normal.x) || isnan(normal.y) || isnan(normal.z)) // happens when the direction vector points up or down?
		normal = { 1, 0, 0 };

	const glm::vec3 perp = glm::normalize(glm::cross(direction, normal));
	Model::vertex_t v;
	v.r = 0;
	v.b = v.g = v.a = 255;

	auto addPoint = [&v, model](const glm::vec3& p)
		{
			v.x = v.oX = (int)p.x;
			v.y = v.oY = (int)p.y;
			v.z = v.oZ = (int)p.z;
			model->vertices.push_back(v);
		};

	addPoint(start + normal * 25.f);
	addPoint(start - normal * 25.f + perp * 25.f);
	addPoint(start - normal * 25.f - perp * 25.f);
	addPoint(end);

	model->polygons.push_back({
		model->vertices.size() - 3,
		model->vertices.size() - 1,
		model->vertices.size() - 4,
		});

	model->polygons.push_back({
		model->vertices.size() - 1,
		model->vertices.size() - 2,
		model->vertices.size() - 4,
		});

	model->polygons.push_back({
		model->vertices.size() - 1,
		model->vertices.size() - 3,
		model->vertices.size() - 2,
		});
}

void CreateSpriteObject(level_t& level, std::shared_ptr<Model> model, const std::string& name, unsigned int customId, int scale)
{
	model->name = name;

	model->vertices.push_back({ -100 * scale,  100 * scale, 0, -100 * scale,   100 * scale, 0, 0, 128, 128, 128, 255 });
	model->vertices.push_back({ 100 * scale,  100 * scale, 0,  100 * scale,   100 * scale, 0, 0, 128, 128, 128, 255 });
	model->vertices.push_back({ 100 * scale, -100 * scale, 0,  100 * scale,  -100 * scale, 0, 0, 128, 128, 128, 255 });
	model->vertices.push_back({ -100 * scale, -100 * scale, 0, -100 * scale,  -100 * scale, 0, 0, 128, 128, 128, 255 });

	model->polygons.push_back({ {2, 1, 0}, customId, 0, {{1, 1}, {1, 0}, {0, 0}} });
	model->polygons.push_back({ {3, 2, 0}, customId, 0, {{0, 1}, {1, 1}, {0, 0}} });
}

ImagePacker::ImageInformation_t* FindImageInfoById(ImagePacker::ImageInformationList& list, int id)
{
	if (auto it = std::find_if(list.begin(), list.end(), [id](const ImagePacker::ImageInformation_t& it)
		{
			return id == (int)it.userdata;
		}); it != list.end())
	{
		return &*it;
	}

	return nullptr;
}

void ApplyPathModels(level_t& level)
{
	for (size_t i = 0; i < level.models.size(); ++i)
		for (auto& inst : level.models[i]->instances)
		{
			Path* pp = NULL;
			for (auto& c : inst.components)
			{
				if (auto pc = dynamic_cast<PathComponent*>(c.get()))
				{
					pp = &pc->path;
					break;
				}
			}
			if (!pp)
				continue;
			Path& p = *pp;

			if (p.translations.size() == 0)
				continue;

			auto mdl = std::make_shared<Model>(p.address);
			mdl->name += "@Path-" + Hexify(p.address);
			level.models.push_back(mdl);
			mdl->instances.push_back({ {0, 0, 0}, {0, 0, 0} });
			mdl->hasNoTextures = true;
			for (size_t i = 0; (i + 1) < p.translations.size(); ++i)
			{
				AddLineToModel(mdl, glm::vec3{
						p.translations[i].x,
						p.translations[i].z,
						-p.translations[i].y,
					}, glm::vec3{
						p.translations[i + 1].x,
						p.translations[i + 1].z,
						-p.translations[i + 1].y,
					});

				AddDiamondToModel(mdl, glm::vec3{
						p.translations[i].x,
						p.translations[i].z,
						-p.translations[i].y,
					}, 50.f);
			}

			AddDiamondToModel(mdl, glm::vec3{
					p.translations.back().x,
					p.translations.back().z,
					-p.translations.back().y,
				}, 50.f);
		}
}