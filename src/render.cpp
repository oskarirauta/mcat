#include <cstdio>
#include "ansi.hpp"
#include "highlight.hpp"

namespace mcat {

	std::vector<std::string> highlight_plain(const std::string& data, const options_t& opts) {
		std::vector<std::string> out;
		for ( auto& l : split_lines(data))
			out.push_back(expand_tabs(l, opts.tabstop));
		return out;
	}

	std::vector<std::string> render(const std::string& data, filetype type, const options_t& opts) {

		std::vector<std::string> lines;

		if ( !opts.color ) {
			// no colours requested: just expand tabs, no markup
			lines = highlight_plain(data, opts);
		} else {
			switch ( type ) {
				case filetype::JSON:     lines = highlight_json(data, opts); break;
				case filetype::MARKDOWN: lines = highlight_markdown(data, opts); break;
				case filetype::CONFIG:   lines = highlight_config(data, opts); break;
				case filetype::CPP:      lines = highlight_cpp(data, opts); break;
				default:                 lines = highlight_plain(data, opts); break;
			}
		}

		if ( opts.numbers ) {
			size_t total = lines.size();
			int w = 1;
			for ( size_t n = total; n >= 10; n /= 10 ) w++;
			std::vector<std::string> numbered;
			numbered.reserve(lines.size());
			for ( size_t i = 0; i < lines.size(); i++ ) {
				std::string num = std::to_string(i + 1);
				if ( (int)num.size() < w ) num = std::string(w - num.size(), ' ') + num;
				num += ' ';
				std::string prefix = opts.color ? ( ansi::grey + num + ansi::reset ) : num;
				numbered.push_back(prefix + lines[i]);
			}
			return numbered;
		}

		return lines;
	}

} // namespace mcat
