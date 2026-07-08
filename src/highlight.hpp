#pragma once
#include <string>
#include <vector>
#include "ansi.hpp"
#include "mcat.hpp"

// Internal declarations shared between the render dispatcher and the
// per-language highlighters. Each highlighter receives the raw file content
// and returns display lines (ANSI-coloured when opts.color is set).
namespace mcat {

	// Append `text` wrapped in `color`, keeping each output line self-contained:
	// a colour that spans a newline is closed before the '\n' and reopened after,
	// so split_lines() yields rows that render correctly on their own.
	inline void emit(std::string& out, const std::string& color, const std::string& text) {
		out += color;
		for ( char c : text ) {
			if ( c == '\n' ) { out += ansi::reset; out += '\n'; out += color; }
			else out += c;
		}
		out += ansi::reset;
	}

	std::vector<std::string> highlight_json(const std::string& data, const options_t& opts);
	std::vector<std::string> highlight_markdown(const std::string& data, const options_t& opts);
	std::vector<std::string> highlight_config(const std::string& data, const options_t& opts);
	std::vector<std::string> highlight_cpp(const std::string& data, const options_t& opts);
	std::vector<std::string> highlight_shell(const std::string& data, const options_t& opts);
	std::vector<std::string> highlight_diff(const std::string& data, const options_t& opts);
	std::vector<std::string> highlight_plain(const std::string& data, const options_t& opts);

} // namespace mcat
