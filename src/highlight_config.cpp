#include <cctype>
#include "highlight.hpp"

namespace mcat {

	static size_t first_nonspace(const std::string& s) {
		size_t i = 0;
		while ( i < s.size() && ( s[i] == ' ' || s[i] == '\t' )) i++;
		return i;
	}

	// colour a config value: quoted string, number, boolean-ish, else plain
	static void emit_value(std::string& out, const std::string& v) {

		size_t a = 0;
		while ( a < v.size() && v[a] == ' ' ) { out += ' '; a++; }
		std::string t = v.substr(a);

		// strip a trailing inline comment starting with " #" or " ;" (common in ini)
		std::string val = t, comment;
		for ( size_t i = 0; i + 1 < t.size(); i++ ) {
			if ( t[i] == ' ' && ( t[i+1] == '#' || t[i+1] == ';' )) {
				val = t.substr(0, i);
				comment = t.substr(i);
				break;
			}
		}

		std::string low;
		for ( char c : val ) low += std::tolower((unsigned char)c);

		if ( !val.empty() && ( val.front() == '"' || val.front() == '\'' )) {
			emit(out, ansi::green, val);
		} else if ( low == "true" || low == "false" || low == "yes" || low == "no" ||
			low == "on" || low == "off" || low == "null" || low == "none" ) {
			emit(out, ansi::b_yellow, val);
		} else {
			bool numeric = !val.empty();
			for ( char c : val )
				if ( !std::isdigit((unsigned char)c) && c != '.' && c != '-' && c != '+' ) { numeric = false; break; }
			if ( numeric ) emit(out, ansi::b_magenta, val);
			else out += val;
		}

		if ( !comment.empty()) emit(out, ansi::grey, comment);
	}

	std::vector<std::string> highlight_config(const std::string& data, const options_t& opts) {

		std::vector<std::string> out;

		for ( auto& raw : split_lines(data)) {

			std::string line = expand_tabs(raw, opts.tabstop);
			size_t p = first_nonspace(line);
			std::string indent = line.substr(0, p);
			std::string body = line.substr(p);

			if ( body.empty()) { out.push_back(line); continue; }

			// full-line comment
			if ( body[0] == '#' || body[0] == ';' ) {
				std::string s;
				emit(s, ansi::grey, line);
				out.push_back(s);
				continue;
			}

			// section header  [name]
			if ( body.front() == '[' && body.back() == ']' ) {
				std::string s = indent;
				emit(s, ansi::b_yellow + ansi::bold, body);
				out.push_back(s);
				continue;
			}

			// yaml/list bullet
			if (( body[0] == '-' ) && body.size() > 1 && body[1] == ' ' ) {
				std::string s = indent;
				emit(s, ansi::yellow, "-");
				emit_value(s, body.substr(1));
				out.push_back(s);
				continue;
			}

			// key <sep> value    (sep is '=' or ':')
			size_t eq = body.find('=');
			size_t co = body.find(':');
			size_t sep = std::string::npos;
			char sepc = 0;
			if ( eq != std::string::npos && ( co == std::string::npos || eq < co )) { sep = eq; sepc = '='; }
			else if ( co != std::string::npos ) { sep = co; sepc = ':'; }

			if ( sep != std::string::npos ) {
				std::string key = body.substr(0, sep);
				std::string val = body.substr(sep + 1);
				std::string s = indent;
				emit(s, ansi::b_cyan, key);
				emit(s, ansi::grey, std::string(1, sepc));
				emit_value(s, val);
				out.push_back(s);
				continue;
			}

			out.push_back(line);
		}

		return out;
	}

} // namespace mcat
