#pragma once
#include <string>
#include <unordered_map>

class JSON
{
	using string = std::string;
	std::unordered_map<string, JSON> m_Map;
	string value_string;
	long long int value_int = 0;
	double value_double = 0;
	enum class value_type : char
	{
		string	=	0b000001,
		integer =	0b000010,
		decimal	=	0b000100,
		boolean	=	0b001000,
		array	=	0b010000,
		object	=	0b100000
	} type = value_type::object;
	void WriteToFileNormal(std::stringstream& ss);
	void WriteToFileReadable(std::stringstream& ss, std::string tab = "");
public:
	JSON() = default;
	template<typename T>
	JSON(std::initializer_list<T> list)
	{
		type = value_type::array;
		for (auto it = list.begin(); it != list.end(); ++it)
			m_Map[std::to_string(m_Map.size())] = *it;
	}

	JSON* Get(const std::string& key) const;
	JSON* Get(const size_t& index) const;

	JSON& operator[](const std::string& key);
	JSON& operator[](const size_t& index);

	// Insert element at end of array
	JSON& push_back(const JSON& jo);

	template<typename T>
	JSON& operator=(T value);

	static JSON Array() { JSON j; j.type = value_type::array; return j; }
	static JSON Int(int i) { JSON j; j.type = value_type::integer; j.value_int = i; return j; }

	// Debatable if readable, but it's definitely easier to read. Also a lot heavier. Prefer setting to false
	void WriteToFile(const string& filePath, bool humanReadable = false);
};