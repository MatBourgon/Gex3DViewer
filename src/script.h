#pragma once
#include <vector>
#include <string>

struct level_t;
struct file_t;
using addr_t = unsigned int;

void ParseCommands(level_t& level, file_t& file, addr_t address, std::vector<std::string>& commands);