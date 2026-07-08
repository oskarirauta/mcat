#include <cctype>
#include "json.hpp"
#include "highlight.hpp"

namespace mcat {

	std::vector<std::string> highlight_json(const std::string& data, const options_t& opts) {

		// Optionally reformat through json_cpp (default). If the document does
		// not parse we silently keep the original text and colour it as-is, so
		// broken/partial json still renders.
		std::string src = data;
		if ( opts.json_reformat ) {
			try {
				JSON doc = JSON::parse(data);
				src = doc.dump(true);
			} catch ( ... ) {
				src = data;
			}
		}

		std::string s = expand_tabs_data(src, opts.tabstop);
		std::string out;
		size_t i = 0, n = s.size();

		while ( i < n ) {

			char c = s[i];

			if ( c == '"' ) {

				size_t start = i;
				i++;
				while ( i < n ) {
					if ( s[i] == '\\' && i + 1 < n ) { i += 2; continue; }
					if ( s[i] == '"' ) { i++; break; }
					if ( s[i] == '\n' ) break; // unterminated - stop at line end
					i++;
				}
				std::string str = s.substr(start, i - start);

				// key if the next non-whitespace char is ':'
				size_t j = i;
				while ( j < n && ( s[j] == ' ' || s[j] == '\n' )) j++;
				bool key = ( j < n && s[j] == ':' );

				emit(out, key ? ansi::b_blue : ansi::green, str);

			} else if ( c == '-' || std::isdigit((unsigned char)c)) {

				size_t start = i;
				if ( c == '-' ) i++;
				while ( i < n && ( std::isdigit((unsigned char)s[i]) || s[i] == '.' ||
					s[i] == 'e' || s[i] == 'E' || s[i] == '+' || s[i] == '-' )) i++;
				emit(out, ansi::b_magenta, s.substr(start, i - start));

			} else if ( std::isalpha((unsigned char)c)) {

				size_t start = i;
				while ( i < n && std::isalpha((unsigned char)s[i])) i++;
				std::string w = s.substr(start, i - start);
				if ( w == "true" || w == "false" ) emit(out, ansi::b_yellow, w);
				else if ( w == "null" ) emit(out, ansi::grey, w);
				else out += w;

			} else if ( c == '{' || c == '}' || c == '[' || c == ']' ) {

				emit(out, ansi::yellow, std::string(1, c));
				i++;

			} else if ( c == ':' || c == ',' ) {

				emit(out, ansi::grey, std::string(1, c));
				i++;

			} else {
				out += c;
				i++;
			}
		}

		return split_lines(out);
	}

} // namespace mcat
