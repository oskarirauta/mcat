#include <cctype>
#include "highlight.hpp"

namespace mcat {

	// inline markup: `code`, **bold**, *italic* / _italic_, [text](url)
	static std::string inline_md(const std::string& s) {

		std::string out;
		size_t i = 0, n = s.size();

		while ( i < n ) {

			char c = s[i];

			if ( c == '`' ) {
				size_t end = s.find('`', i + 1);
				if ( end != std::string::npos ) {
					emit(out, ansi::b_cyan, s.substr(i, end - i + 1));
					i = end + 1;
					continue;
				}
			} else if ( c == '*' && i + 1 < n && s[i+1] == '*' ) {
				size_t end = s.find("**", i + 2);
				if ( end != std::string::npos ) {
					emit(out, ansi::bold + ansi::b_white, s.substr(i, end - i + 2));
					i = end + 2;
					continue;
				}
			} else if (( c == '*' || c == '_' )) {
				size_t end = s.find(c, i + 1);
				if ( end != std::string::npos && end > i + 1 ) {
					emit(out, ansi::italic, s.substr(i, end - i + 1));
					i = end + 1;
					continue;
				}
			} else if ( c == '[' ) {
				size_t rb = s.find(']', i + 1);
				if ( rb != std::string::npos && rb + 1 < n && s[rb+1] == '(' ) {
					size_t rp = s.find(')', rb + 2);
					if ( rp != std::string::npos ) {
						emit(out, ansi::b_blue + ansi::under, s.substr(i, rb - i + 1)); // [text]
						emit(out, ansi::grey, s.substr(rb + 1, rp - rb));               // (url)
						i = rp + 1;
						continue;
					}
				}
			}

			out += c;
			i++;
		}
		return out;
	}

	std::vector<std::string> highlight_markdown(const std::string& data, const options_t& opts) {

		std::vector<std::string> out;
		bool in_fence = false;

		for ( auto& raw : split_lines(data)) {

			std::string line = expand_tabs(raw, opts.tabstop);

			// fenced code block ``` or ~~~
			size_t p = 0;
			while ( p < line.size() && line[p] == ' ' ) p++;
			bool is_fence = line.compare(p, 3, "```") == 0 || line.compare(p, 3, "~~~") == 0;

			if ( is_fence ) {
				in_fence = !in_fence;
				std::string s;
				emit(s, ansi::grey, line);
				out.push_back(s);
				continue;
			}

			if ( in_fence ) {
				std::string s;
				emit(s, ansi::b_green, line);
				out.push_back(s);
				continue;
			}

			std::string body = line.substr(p);
			std::string indent = line.substr(0, p);

			if ( body.empty()) { out.push_back(line); continue; }

			// heading
			if ( body[0] == '#' ) {
				std::string s;
				emit(s, ansi::bold + ansi::b_cyan, line);
				out.push_back(s);
				continue;
			}

			// blockquote
			if ( body[0] == '>' ) {
				std::string s;
				emit(s, ansi::grey + ansi::italic, line);
				out.push_back(s);
				continue;
			}

			// horizontal rule
			if ( body == "---" || body == "***" || body == "___" ||
				body == "- - -" || body == "* * *" ) {
				std::string s;
				emit(s, ansi::grey, line);
				out.push_back(s);
				continue;
			}

			// unordered list  -, *, +
			if (( body[0] == '-' || body[0] == '*' || body[0] == '+' ) &&
				body.size() > 1 && body[1] == ' ' ) {
				std::string s = indent;
				emit(s, ansi::b_yellow, std::string(1, body[0]));
				s += inline_md(body.substr(1));
				out.push_back(s);
				continue;
			}

			// ordered list  1. 2. ...
			{
				size_t d = 0;
				while ( d < body.size() && std::isdigit((unsigned char)body[d])) d++;
				if ( d > 0 && d < body.size() && body[d] == '.' &&
					d + 1 < body.size() && body[d+1] == ' ' ) {
					std::string s = indent;
					emit(s, ansi::b_yellow, body.substr(0, d + 1));
					s += inline_md(body.substr(d + 1));
					out.push_back(s);
					continue;
				}
			}

			out.push_back(indent + inline_md(body));
		}

		return out;
	}

} // namespace mcat
