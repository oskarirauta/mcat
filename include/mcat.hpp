#pragma once
#include <string>
#include <vector>

namespace mcat {

	enum class filetype {
		PLAIN,
		JSON,
		MARKDOWN,
		CONFIG,
		CPP,
		SHELL,
		DIFF,
		BINARY,
		UNKNOWN
	};

	std::string typname(filetype t);

	struct options_t {
		bool color = true;        // emit ANSI colour codes
		bool paging = true;       // browser mode allowed (when output is a tty)
		bool force_pager = false; // -b: always page
		bool numbers = false;     // -n: line numbers
		int tabstop = 8;          // tab expansion width
		bool json_reformat = true; // reformat json through json_cpp (off with -r/--raw)
		filetype force_type = filetype::UNKNOWN; // -t <type>: override detection
	};

	// --- detect.cpp ---
	bool is_binary(const std::string& data);
	filetype detect_type(const std::string& path, const std::string& data);
	filetype type_from_name(const std::string& name); // by extension only, UNKNOWN if unsure
	std::string binary_info(const std::string& path, const std::string& data);

	// --- render.cpp / highlight_*.cpp ---
	// Turn raw file content into a vector of display lines (ANSI-coloured when
	// opts.color is set). One element == one screen row before truncation.
	std::vector<std::string> render(const std::string& data, filetype type, const options_t& opts);

	// --- pager.cpp ---
	void run_pager(const std::vector<std::string>& lines, const std::string& title);

	// --- util.cpp ---
	size_t visible_width(const std::string& s);          // width ignoring ANSI escapes
	std::string clip_visible(const std::string& s, size_t maxw); // truncate to width, keep escapes
	std::string clip_visible_range(const std::string& s, size_t start, size_t width); // horizontal window
	std::string plain_text(const std::string& s);        // strip ANSI escapes (for searching)
	std::string highlight_matches(const std::string& s, const std::string& query_lower); // reverse-video matches
	std::string expand_tabs(const std::string& line, int tabstop);
	std::string expand_tabs_data(const std::string& data, int tabstop); // whole text, resets per line
	std::vector<std::string> split_lines(const std::string& data);

} // namespace mcat
