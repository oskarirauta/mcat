#include "highlight.hpp"

namespace mcat {

	std::vector<std::string> highlight_diff(const std::string& data, const options_t& opts) {

		std::vector<std::string> out;

		for ( auto& raw : split_lines(data)) {

			std::string line = expand_tabs(raw, opts.tabstop);
			std::string s;

			if ( line.empty()) { out.push_back(line); continue; }

			// file markers first (--- / +++), before +/- body handling
			if ( line.compare(0, 4, "--- ") == 0 || line.compare(0, 4, "+++ ") == 0 ) {
				emit(s, ansi::bold + ansi::b_yellow, line);
			} else if ( line.compare(0, 2, "@@") == 0 ) {
				emit(s, ansi::b_cyan, line);            // hunk header
			} else if ( line.compare(0, 5, "diff ") == 0 ||
				line.compare(0, 6, "index ") == 0 ||
				line.compare(0, 10, "similarity") == 0 ||
				line.compare(0, 6, "rename") == 0 ||
				line.compare(0, 4, "new ") == 0 ||
				line.compare(0, 8, "deleted ") == 0 ||
				line.compare(0, 7, "Index: ") == 0 ) {
				emit(s, ansi::bold + ansi::yellow, line); // git metadata
			} else if ( line[0] == '+' ) {
				emit(s, ansi::b_green, line);            // added
			} else if ( line[0] == '-' ) {
				emit(s, ansi::b_red, line);              // removed
			} else if ( line[0] == '\\' ) {
				emit(s, ansi::grey, line);               // "\ No newline at end of file"
			} else {
				emit(s, ansi::dim, line);                // context
			}

			out.push_back(s);
		}

		return out;
	}

} // namespace mcat
