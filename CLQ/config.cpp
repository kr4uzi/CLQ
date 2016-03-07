#include "config.h"
#include <fstream>
#include <algorithm>	// remove

config::parse_error config::parse(const std::string& filename, config& cfg)
{
	std::ifstream in;
	in.open(filename);

	if (in.fail())
		return parse_error("unable to open config file");
	else
	{
		std::string line;

		while (!in.eof())
		{
			std::getline(in, line);

			// ignore blank lines and comments
			if (line.empty() || line[0] == '#')
				continue;

			// remove newlines and partial newlines to help fix issues with Windows formatted config files on Linux systems
			line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
			line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());

			std::string::size_type split = line.find("=");

			if (split == std::string::npos)
				continue;

			std::string::size_type key_start = line.find_first_not_of(" ");
			std::string::size_type key_end = line.find(" ", key_start);
			std::string::size_type value_start = line.find_first_not_of(" ", split + 1);
			std::string::size_type value_end = line.size();

			if (value_start != std::string::npos)
				cfg.cfg[line.substr(key_start, key_end - key_start)] = line.substr(value_start, value_end - value_start);
		}

		in.close();
	}

	return parse_error("");
}

bool config::exists(const std::string& key) const
{
	return cfg.find(key) != cfg.end();
}

std::string config::get_string(const std::string& key, const std::string& default_value) const
{
	if (!exists(key)) return default_value;
	return cfg.find(key)->second;
}

double config::get_double(const std::string& key, double default_value) const
{
	if (!exists(key)) return default_value;
	return std::stod(cfg.find(key)->second);
}

unsigned config::get_unsinged(const std::string& key, unsigned default_value) const
{
	if (!exists(key)) return default_value;
	return std::stoul(cfg.find(key)->second);
}

signed config::get_signed(const std::string& key, signed default_value) const
{
	if (!exists(key)) return default_value;
	return std::stol(cfg.find(key)->second);
}

bool config::get_bool(const std::string& key, bool default_value) const
{
	if (!exists(key)) return default_value;

	auto value = cfg.find(key)->second;
	if (value == "True" || value == "true" || value == "1")
		return true;
	
	return false;
}