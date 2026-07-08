#include <cctype>
#include <unordered_set>
#include "highlight.hpp"

namespace mcat {

	static const std::unordered_set<std::string>& sh_keywords() {
		static const std::unordered_set<std::string> kw = {
			"if", "then", "else", "elif", "fi", "for", "while", "until", "do",
			"done", "case", "esac", "in", "function", "select", "time",
			"return", "break", "continue", "exit", "local", "declare", "typeset",
			"readonly", "export", "unset", "shift", "set", "eval", "exec",
			"source", "alias", "unalias", "trap", "test"
		};
		return kw;
	}

	static const std::unordered_set<std::string>& sh_builtins() {
		static const std::unordered_set<std::string> b = {
			"echo", "printf", "read", "cd", "pwd", "pushd", "popd", "dirs",
			"true", "false", "let", "getopts", "wait", "kill", "jobs", "fg",
			"bg", "type", "command", "builtin", "hash", "umask", "ulimit"
		};
		return b;
	}

	std::vector<std::string> highlight_shell(const std::string& data, const options_t& opts) {

		std::string s = expand_tabs_data(data, opts.tabstop);
		std::string out;
		size_t i = 0, n = s.size();
		bool prev_space = true; // start of line counts as whitespace boundary

		// shebang on the very first line
		if ( s.compare(0, 2, "#!") == 0 ) {
			size_t eol = s.find('\n');
			size_t len = eol == std::string::npos ? n : eol;
			emit(out, ansi::b_magenta + ansi::bold, s.substr(0, len));
			i = len;
		}

		while ( i < n ) {

			char c = s[i];

			if ( c == '\n' ) { out += c; i++; prev_space = true; continue; }
			if ( c == ' ' ) { out += c; i++; prev_space = true; continue; }

			// comment: '#' at a word boundary
			if ( c == '#' && prev_space ) {
				size_t start = i;
				while ( i < n && s[i] != '\n' ) i++;
				emit(out, ansi::grey, s.substr(start, i - start));
				continue;
			}

			// single-quoted string (literal)
			if ( c == '\'' ) {
				size_t start = i;
				i++;
				while ( i < n && s[i] != '\'' ) i++;
				if ( i < n ) i++;
				emit(out, ansi::green, s.substr(start, i - start));
				prev_space = false;
				continue;
			}

			// double-quoted string (may contain escapes)
			if ( c == '"' ) {
				size_t start = i;
				i++;
				while ( i < n ) {
					if ( s[i] == '\\' && i + 1 < n ) { i += 2; continue; }
					if ( s[i] == '"' ) { i++; break; }
					if ( s[i] == '\n' ) break;
					i++;
				}
				emit(out, ansi::green, s.substr(start, i - start));
				prev_space = false;
				continue;
			}

			// variable: $name, ${...}, $1, $?, $@ ...
			if ( c == '$' && i + 1 < n ) {
				size_t start = i;
				i++;
				if ( s[i] == '{' ) {
					while ( i < n && s[i] != '}' ) i++;
					if ( i < n ) i++;
				} else if ( std::isalnum((unsigned char)s[i]) || s[i] == '_' ) {
					while ( i < n && ( std::isalnum((unsigned char)s[i]) || s[i] == '_' )) i++;
				} else {
					i++; // special single-char var like $?, $@, $#
				}
				emit(out, ansi::b_cyan, s.substr(start, i - start));
				prev_space = false;
				continue;
			}

			// word: keyword / builtin / plain
			if ( std::isalpha((unsigned char)c) || c == '_' ) {
				size_t start = i;
				while ( i < n && ( std::isalnum((unsigned char)s[i]) || s[i] == '_' )) i++;
				std::string w = s.substr(start, i - start);
				if ( sh_keywords().count(w)) emit(out, ansi::b_blue + ansi::bold, w);
				else if ( sh_builtins().count(w)) emit(out, ansi::b_blue, w);
				else out += w;
				prev_space = false;
				continue;
			}

			out += c;
			i++;
			prev_space = false;
		}

		return split_lines(out);
	}

} // namespace mcat
