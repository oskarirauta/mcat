#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/ioctl.h>

#include "usage.hpp"
#include "ansi.hpp"
#include "mcat.hpp"
#include "version.hpp"

static int term_rows() {
	struct winsize ws;
	if ( ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 )
		return ws.ws_row;
	return 24;
}

static bool read_file(const std::string& path, std::string& out) {
	if ( path == "-" ) {
		std::ostringstream ss;
		ss << std::cin.rdbuf();
		out = ss.str();
		return true;
	}
	std::ifstream f(path, std::ios::binary);
	if ( !f ) return false;
	std::ostringstream ss;
	ss << f.rdbuf();
	out = ss.str();
	return true;
}

static mcat::filetype type_from_string(const std::string& s) {
	if ( s == "json" ) return mcat::filetype::JSON;
	if ( s == "md" || s == "markdown" ) return mcat::filetype::MARKDOWN;
	if ( s == "config" || s == "conf" || s == "ini" ) return mcat::filetype::CONFIG;
	if ( s == "c" || s == "cpp" || s == "c++" ) return mcat::filetype::CPP;
	if ( s == "sh" || s == "shell" || s == "bash" ) return mcat::filetype::SHELL;
	if ( s == "diff" || s == "patch" ) return mcat::filetype::DIFF;
	if ( s == "text" || s == "plain" ) return mcat::filetype::PLAIN;
	return mcat::filetype::UNKNOWN;
}

int main(int argc, char **argv) {

	usage_t usage = {
		.args = { argc, argv },
		.info = {
			.name = "mcat",
			.version = VERSION,
			.author = "Oskari Rauta",
			.copyright = "2026, Oskari Rauta",
			.usage = "[options] <file>...",
			.description = "\nmarkup-aware cat: syntax colours for json, markdown, config, c/c++,\n"
				"shell and diff/patch, binary detection, and a built-in browser (pager)\n"
				"for long files."
		},
		.options = {
			{ "numbers", { .key = "n", .word = "number", .desc = "show line numbers" }},
			{ "browser", { .key = "b", .word = "browser", .desc = "always use the browser (pager)" }},
			{ "nopager", { .key = "p", .word = "no-pager", .desc = "disable the browser: print everything and exit" }},
			{ "nocolor", { .key = "C", .word = "no-color", .desc = "disable colours" }},
			{ "raw", { .key = "r", .word = "raw", .desc = "json: don't reformat, colour the file as-is" }},
			{ "type", { .key = "t", .word = "type", .desc = "force type: json|md|config|cpp|sh|diff|text",
				.flag = usage_t::REQUIRED, .name = "type" }},
			{ "tab", { .key = "T", .word = "tab", .desc = "tab width (default 8)",
				.flag = usage_t::REQUIRED, .name = "n", .type = usage_t::INT }},
			{ "help", { .key = "h", .word = "help", .desc = "usage help" }},
			{ "version", { .key = "v", .word = "version", .desc = "show version" }}
		}
	};

	if ( usage["help"] ) {
		std::cout << usage << "\n" << usage.help() << "\n" << std::endl;
		return 0;
	} else if ( usage["version"] ) {
		std::cout << usage.version() << std::endl;
		return 0;
	}

	if ( !usage.validated ) {
		auto errors = usage.errors();
		std::cerr << "command-line errors:\n" << errors << std::endl;
		if ( auto it = std::find_if(errors.begin(), errors.end(),
			[](usage_t::error_t e) {
				return e.error != usage_t::error_type::DUPLICATE &&
					e.error != usage_t::error_type::UNKNOWN_OPTION;
			}); it != errors.end()) {
			std::cerr << "aborting." << std::endl;
			return 1;
		}
	}

	std::vector<std::string> files = usage.remainder();
	if ( files.empty()) {
		std::cerr << usage.name() << ": no input files (try --help)" << std::endl;
		return 1;
	}

	bool tty_out = isatty(STDOUT_FILENO);

	mcat::options_t opts;
	opts.numbers = (bool)usage["numbers"];
	opts.force_pager = (bool)usage["browser"];
	opts.color = ( tty_out || opts.force_pager ) && !usage["nocolor"];
	opts.paging = !usage["nopager"] && ( tty_out || opts.force_pager );
	if ( usage["tab"] ) opts.tabstop = (int)usage["tab"].intValue();
	opts.json_reformat = !usage["raw"];
	if ( usage["type"] )
		opts.force_type = type_from_string((std::string)usage["type"]);

	bool multi = files.size() > 1;
	std::vector<std::string> all;   // combined display lines (for paging)
	std::string pager_title;
	int rc = 0;

	for ( size_t fi = 0; fi < files.size(); fi++ ) {

		const std::string& path = files[fi];
		std::string data;

		if ( !read_file(path, data)) {
			std::cerr << "mcat: " << path << ": cannot open file" << std::endl;
			rc = 1;
			continue;
		}

		mcat::filetype type = opts.force_type != mcat::filetype::UNKNOWN
			? opts.force_type
			: mcat::detect_type(path, data);

		// header when several files (cat -style)
		if ( multi ) {
			std::string hdr = "==> " + path + " <==";
			if ( opts.color ) hdr = ansi::bold + ansi::b_yellow + hdr + ansi::reset;
			if ( fi > 0 ) all.push_back("");
			all.push_back(hdr);
		}

		if ( type == mcat::filetype::BINARY ) {
			std::string info = mcat::binary_info(path, data);
			if ( opts.color ) info = ansi::b_red + info + ansi::reset;
			// split so it participates in paging too
			for ( auto& l : mcat::split_lines(info)) all.push_back(l);
			continue;
		}

		std::vector<std::string> lines = mcat::render(data, type, opts);
		for ( auto& l : lines ) all.push_back(std::move(l));

		if ( pager_title.empty()) pager_title = path;
	}

	if ( multi ) pager_title = "mcat: " + std::to_string(files.size()) + " files";

	// decide paging
	int rows = term_rows();
	bool page = opts.paging && ( opts.force_pager || (int)all.size() > rows - 1 );

	if ( page ) {
		mcat::run_pager(all, pager_title.empty() ? "mcat" : pager_title);
	} else {
		for ( auto& l : all ) {
			std::fputs(l.c_str(), stdout);
			std::fputc('\n', stdout);
		}
	}

	return rc;
}
