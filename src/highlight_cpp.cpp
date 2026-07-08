#include <cctype>
#include <unordered_set>
#include "highlight.hpp"

namespace mcat {

	static const std::unordered_set<std::string>& keywords() {
		static const std::unordered_set<std::string> kw = {
			// control / statements
			"if", "else", "for", "while", "do", "switch", "case", "default",
			"break", "continue", "return", "goto",
			// types & qualifiers
			"int", "char", "short", "long", "float", "double", "void", "bool",
			"signed", "unsigned", "size_t", "wchar_t", "auto", "const", "constexpr",
			"volatile", "static", "extern", "register", "inline", "mutable",
			"struct", "union", "enum", "class", "typedef", "using", "namespace",
			"template", "typename", "public", "private", "protected", "virtual",
			"friend", "operator", "explicit", "override", "final", "noexcept",
			"new", "delete", "this", "nullptr", "true", "false", "sizeof",
			"try", "catch", "throw", "and", "or", "not",
			"static_cast", "dynamic_cast", "reinterpret_cast", "const_cast",
			"decltype", "typeid", "std", "uint8_t", "uint16_t", "uint32_t",
			"uint64_t", "int8_t", "int16_t", "int32_t", "int64_t"
		};
		return kw;
	}

	std::vector<std::string> highlight_cpp(const std::string& data, const options_t& opts) {

		std::string s = expand_tabs_data(data, opts.tabstop);
		std::string out;
		size_t i = 0, n = s.size();
		bool at_line_start = true;

		while ( i < n ) {

			char c = s[i];

			// track logical start of line (ignoring leading whitespace)
			if ( c == '\n' ) { out += c; i++; at_line_start = true; continue; }
			if ( c == ' ' ) { out += c; i++; continue; }

			// preprocessor directive - colour to end of (possibly continued) line
			if ( at_line_start && c == '#' ) {
				size_t start = i;
				while ( i < n ) {
					if ( s[i] == '\n' ) {
						// line continuation with backslash keeps the directive going
						if ( i > start && s[i-1] == '\\' ) { i++; continue; }
						break;
					}
					i++;
				}
				emit(out, ansi::b_magenta, s.substr(start, i - start));
				at_line_start = false;
				continue;
			}
			at_line_start = false;

			// line comment
			if ( c == '/' && i + 1 < n && s[i+1] == '/' ) {
				size_t start = i;
				while ( i < n && s[i] != '\n' ) i++;
				emit(out, ansi::grey, s.substr(start, i - start));
				continue;
			}

			// block comment (may span lines)
			if ( c == '/' && i + 1 < n && s[i+1] == '*' ) {
				size_t start = i;
				i += 2;
				while ( i < n && !( s[i] == '*' && i + 1 < n && s[i+1] == '/' )) i++;
				if ( i < n ) i += 2; // consume closing */
				emit(out, ansi::grey, s.substr(start, i - start));
				continue;
			}

			// string / char literal
			if ( c == '"' || c == '\'' ) {
				char q = c;
				size_t start = i;
				i++;
				while ( i < n ) {
					if ( s[i] == '\\' && i + 1 < n ) { i += 2; continue; }
					if ( s[i] == q ) { i++; break; }
					if ( s[i] == '\n' ) break;
					i++;
				}
				emit(out, ansi::green, s.substr(start, i - start));
				continue;
			}

			// number
			if ( std::isdigit((unsigned char)c) ||
				( c == '.' && i + 1 < n && std::isdigit((unsigned char)s[i+1]))) {
				size_t start = i;
				while ( i < n && ( std::isalnum((unsigned char)s[i]) || s[i] == '.' ||
					s[i] == 'x' || s[i] == 'X' || s[i] == '_' )) i++;
				emit(out, ansi::b_magenta, s.substr(start, i - start));
				continue;
			}

			// identifier / keyword
			if ( std::isalpha((unsigned char)c) || c == '_' ) {
				size_t start = i;
				while ( i < n && ( std::isalnum((unsigned char)s[i]) || s[i] == '_' )) i++;
				std::string w = s.substr(start, i - start);
				if ( keywords().count(w)) emit(out, ansi::b_blue, w);
				else out += w;
				continue;
			}

			// punctuation
			out += c;
			i++;
		}

		return split_lines(out);
	}

} // namespace mcat
