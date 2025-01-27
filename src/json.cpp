#include "json.h"

#include <sstream>

JSON* JSON::Get(const std::string& key) const
{
	if (auto it = m_Map.find(key); it != m_Map.end())
		return const_cast<JSON*>(&it->second);
	return nullptr;
}

JSON* JSON::Get(const size_t& index) const
{
	if (type != value_type::array)
		return nullptr;

	return Get(std::to_string(index));
}

JSON& JSON::operator[](const std::string& key)
{
	if ((((unsigned char)type) & 0b110000) == 0) // if not array or object
		type = value_type::object;
	return m_Map[key]; // Create the entry if it didn't exist
}

JSON& JSON::operator[](const size_t& index)
{
	if (type != value_type::array)
	{
		type = value_type::array;
		m_Map.clear();
	}

	while (m_Map.size() < index)
		push_back(Int(0));

	return m_Map[std::to_string(index)];
}

JSON& JSON::push_back(const JSON& jo)
{
	if (type != value_type::array)
	{
		type = value_type::array;
		m_Map.clear();
	}
	auto& r = (*this)[m_Map.size()];
	r = jo;
	return r;
}

template<>
JSON& JSON::operator=(double value)
{
	type = value_type::decimal;
	value_double = value;
	m_Map.clear();
	return *this;
}

template<>
JSON& JSON::operator=(float value)
{
	type = value_type::decimal;
	value_double = value;
	m_Map.clear();
	return *this;
}

template<>
JSON& JSON::operator=(bool value)
{
	type = value_type::boolean;
	value_int = value;
	m_Map.clear();
	return *this;
}

template<>
JSON& JSON::operator=(std::string value)
{
	type = value_type::string;
	value_string = value;
	m_Map.clear();
	return *this;
}

template<>
JSON& JSON::operator=(const char* value)
{
	type = value_type::string;
	value_string = value;
	m_Map.clear();
	return *this;
}

template<>
JSON& JSON::operator=(const JSON& value)
{
	type = value.type;
	m_Map.clear();
	switch (type)
	{
	case value_type::string:
		value_string = value.value_string;
		break;
	case value_type::boolean:
	case value_type::integer:
		value_int = value.value_int;
		break;
	case value_type::decimal:
		value_double = value.value_double;
		break;
	case value_type::object:
	case value_type::array:
		m_Map = value.m_Map;
		break;
	}
	return *this;
}

template<>
JSON& JSON::operator=(long long int value)
{
	type = value_type::integer;
	value_int = value;
	m_Map.clear();
	return *this;
}

template<>
JSON& JSON::operator=(int value)
{
	type = value_type::integer;
	value_int = value;
	m_Map.clear();
	return *this;
}

template<>
JSON& JSON::operator=(short value)
{
	type = value_type::integer;
	value_int = value;
	m_Map.clear();
	return *this;
}

template<>
JSON& JSON::operator=(char value)
{
	type = value_type::integer;
	value_int = value;
	m_Map.clear();
	return *this;
}

template<>
JSON& JSON::operator=(unsigned long long int value)
{
	type = value_type::integer;
	value_int = value;
	m_Map.clear();
	return *this;
}

template<>
JSON& JSON::operator=(unsigned int value)
{
	type = value_type::integer;
	value_int = value;
	m_Map.clear();
	return *this;
}

template<>
JSON& JSON::operator=(unsigned short value)
{
	type = value_type::integer;
	value_int = value;
	m_Map.clear();
	return *this;
}

template<>
JSON& JSON::operator=(unsigned char value)
{
	type = value_type::integer;
	value_int = value;
	m_Map.clear();
	return *this;
}

void JSON::WriteToFileNormal(std::stringstream& ss)
{
	switch (type)
	{
	case value_type::string:
		ss << '"' << value_string << '"';
		break;
	case value_type::decimal:
		ss << value_double;
		break;
	case value_type::integer:
		ss << value_int;
		break;
	case value_type::boolean:
		ss << ((value_int != 0) ? "true" : "false");
		break;
	case value_type::array:
		ss << '[';
		for (size_t i = 0; i < m_Map.size(); ++i)
		{
			Get(i)->WriteToFileNormal(ss);
			if ((i + 1) < m_Map.size())
			{
				ss << ",";
			}
		}
		ss << ']';
		break;
	case value_type::object:
		ss << '{';
		size_t i = 0;
		for (auto& k : m_Map)
		{
			ss << '"' << k.first << "\":";
			k.second.WriteToFileNormal(ss);
			if ((++i) < m_Map.size())
			{
				ss << ',';
			}
		}
		ss << '}';
		break;
	}
}

void JSON::WriteToFileReadable(std::stringstream& ss, std::string tab)
{
	switch (type)
	{
	case value_type::string:
		ss << tab << '"' << value_string << '"';
		break;
	case value_type::decimal:
		ss << tab << value_double;
		break;
	case value_type::integer:
		ss << tab << value_int;
		break;
	case value_type::boolean:
		ss << tab << ((value_int != 0) ? "true" : "false");
		break;
	case value_type::array:
	{
		ss << tab << "[";
		bool wasObject = true;
		for (size_t i = 0; i < m_Map.size(); ++i)
		{
			bool isObj = ((unsigned char)Get(i)->type) & 0b110000;
			if (isObj)
			{
				wasObject = true;
				ss << '\n';
			}
			else
			{
				if (wasObject)
				{
					ss << '\n' << tab + "\t";
					wasObject = false;
				}
				else
					ss << ' ';
			}
			Get(i)->WriteToFileReadable(ss, isObj ? (tab + "\t") : "");
			if ((i + 1) < m_Map.size())
			{
				ss << ",";
			}
		}
		ss << "\n" << tab << ']';
		break;
	}
	case value_type::object:
		ss << tab << "{\n";
		size_t i = 0;
		for (auto& k : m_Map)
		{
			ss << tab << '\t' << '"' << k.first << "\":";
			bool isObj = ((unsigned char)k.second.type) & 0b110000;
			if (isObj)
				ss << '\n';
			else
				ss << ' ';
			k.second.WriteToFileReadable(ss, isObj ? (tab + "\t") : "");
			if ((++i) < m_Map.size())
			{
				ss << ",\n";
			}
		}
		ss << "\n" << tab << '}';
		break;
	}
}

void JSON::WriteToFile(const std::string& filePath, bool humanReadable)
{
	FILE* f = NULL;
	fopen_s(&f, filePath.c_str(), "w");
	if (f)
	{
		std::stringstream ss;
		ss.unsetf(std::ios::skipws);
		if (humanReadable)
			WriteToFileReadable(ss);
		else
			WriteToFileNormal(ss);
		std::string s = ss.str();
		fwrite(s.data(), s.length(), 1, f);
		fclose(f);
	}
}