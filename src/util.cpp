#include "mcat.hpp"

namespace mcat {

	std::string typname(filetype t) {
		switch ( t ) {
			case filetype::JSON:     return "json";
			case filetype::MARKDOWN: return "markdown";
			case filetype::CONFIG:   return "config";
			case filetype::CPP:      return "c/c++";
			case filetype::SHELL:    return "shell";
			case filetype::DIFF:     return "diff";
			case filetype::BINARY:   return "binary";
			case filetype::PLAIN:    return "text";
			default:                 return "unknown";
		}
	}

	std::vector<std::string> split_lines(const std::string& data) {

		std::vector<std::string> lines;
		std::string cur;

		for ( char c : data ) {

			if ( c == '\n' ) {
				lines.push_back(cur);
				cur.clear();
			} else if ( c != '\r' ) {
				cur += c;
			}
		}

		// trailing text without newline still counts as a line;
		// but a trailing newline should not create a spurious empty line.
		if ( !cur.empty()) lines.push_back(cur);

		if ( lines.empty()) lines.push_back("");
		return lines;
	}

	std::string expand_tabs(const std::string& line, int tabstop) {

		if ( tabstop <= 0 ) tabstop = 8;
		std::string out;
		size_t col = 0;

		for ( char c : line ) {
			if ( c == '\t' ) {
				size_t n = tabstop - ( col % tabstop );
				out.append(n, ' ');
				col += n;
			} else {
				out += c;
				col++;
			}
		}
		return out;
	}

	std::string expand_tabs_data(const std::string& data, int tabstop) {
		std::string out;
		size_t col = 0;
		if ( tabstop <= 0 ) tabstop = 8;
		for ( char c : data ) {
			if ( c == '\t' ) {
				size_t n = tabstop - ( col % tabstop );
				out.append(n, ' ');
				col += n;
			} else if ( c == '\n' ) {
				out += c;
				col = 0;
			} else if ( c == '\r' ) {
				// drop CR
			} else {
				out += c;
				col++;
			}
		}
		return out;
	}

	// walk a string, treating "\033[ ... m" (and other CSI) as zero width.
	size_t visible_width(const std::string& s) {

		size_t w = 0;
		for ( size_t i = 0; i < s.size(); i++ ) {

			if ( s[i] == '\033' ) {
				// skip escape sequence: ESC [ ... final-byte
				i++;
				if ( i < s.size() && s[i] == '[' ) {
					i++;
					while ( i < s.size() && !( s[i] >= 0x40 && s[i] <= 0x7e )) i++;
				}
				continue;
			}

			unsigned char c = (unsigned char)s[i];
			// count UTF-8 continuation bytes as zero width (approx: 1 col / codepoint)
			if (( c & 0xc0 ) == 0x80 ) continue;
			w++;
		}
		return w;
	}

	std::string clip_visible(const std::string& s, size_t maxw) {

		if ( visible_width(s) <= maxw ) return s;

		std::string out;
		size_t w = 0;
		bool truncated = false;

		for ( size_t i = 0; i < s.size(); i++ ) {

			if ( s[i] == '\033' ) {
				out += s[i];
				i++;
				if ( i < s.size() && s[i] == '[' ) {
					out += s[i];
					i++;
					while ( i < s.size() && !( s[i] >= 0x40 && s[i] <= 0x7e )) { out += s[i]; i++; }
					if ( i < s.size()) out += s[i];
				} else if ( i < s.size()) out += s[i];
				continue;
			}

			unsigned char c = (unsigned char)s[i];
			if (( c & 0xc0 ) == 0x80 ) { out += s[i]; continue; }

			if ( w >= maxw ) { truncated = true; break; }
			out += s[i];
			w++;
		}

		if ( truncated ) out += "\033[0m";
		return out;
	}

	// Return the visible-column window [start, start+width) of s, keeping ANSI
	// styles: the SGR state active at the cut point is re-emitted so the first
	// visible cell has the right colour even when we scrolled past its escape.
	std::string clip_visible_range(const std::string& s, size_t start, size_t width) {

		std::string out, active;
		size_t col = 0, shown = 0;
		bool started = false;

		for ( size_t i = 0; i < s.size(); i++ ) {

			if ( s[i] == '\033' ) {
				std::string esc;
				esc += s[i];
				i++;
				if ( i < s.size() && s[i] == '[' ) {
					esc += s[i];
					i++;
					while ( i < s.size() && !( s[i] >= 0x40 && s[i] <= 0x7e )) { esc += s[i]; i++; }
					if ( i < s.size()) esc += s[i];
				} else if ( i < s.size()) esc += s[i];

				if ( esc == "\033[0m" ) active.clear();
				else active += esc;
				if ( started && shown < width ) out += esc;
				continue;
			}

			unsigned char c = (unsigned char)s[i];
			if (( c & 0xc0 ) == 0x80 ) { // utf-8 continuation
				if ( started && shown <= width ) out += s[i];
				continue;
			}

			if ( col >= start ) {
				if ( !started ) { started = true; if ( !active.empty()) out += active; }
				if ( shown >= width ) break;
				out += s[i];
				shown++;
			}
			col++;
		}

		out += "\033[0m";
		return out;
	}

	std::string plain_text(const std::string& s) {

		std::string out;
		for ( size_t i = 0; i < s.size(); i++ ) {
			if ( s[i] == '\033' ) {
				i++;
				if ( i < s.size() && s[i] == '[' ) {
					i++;
					while ( i < s.size() && !( s[i] >= 0x40 && s[i] <= 0x7e )) i++;
				}
				continue;
			}
			out += s[i];
		}
		return out;
	}

} // namespace mcat
