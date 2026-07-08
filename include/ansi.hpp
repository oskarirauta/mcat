#pragma once
#include <string>

// Small ANSI palette. Kept minimal on purpose - "enough" colours to be useful.
namespace ansi {

	static const std::string reset   = "\033[0m";
	static const std::string bold     = "\033[1m";
	static const std::string dim      = "\033[2m";
	static const std::string italic   = "\033[3m";
	static const std::string under    = "\033[4m";
	static const std::string reverse  = "\033[7m";

	static const std::string black   = "\033[30m";
	static const std::string red     = "\033[31m";
	static const std::string green   = "\033[32m";
	static const std::string yellow  = "\033[33m";
	static const std::string blue    = "\033[34m";
	static const std::string magenta = "\033[35m";
	static const std::string cyan    = "\033[36m";
	static const std::string white   = "\033[37m";

	static const std::string b_red     = "\033[91m";
	static const std::string b_green   = "\033[92m";
	static const std::string b_yellow  = "\033[93m";
	static const std::string b_blue    = "\033[94m";
	static const std::string b_magenta = "\033[95m";
	static const std::string b_cyan    = "\033[96m";
	static const std::string b_white   = "\033[97m";
	static const std::string grey      = "\033[90m";

} // namespace ansi
