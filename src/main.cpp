#include <iostream>

#include "usage.hpp"
#include "version.hpp"

int main(int argc, char **argv) {

	usage_t usage = {
		.args = { argc, argv },
		.info = {
			.name = "mcat",
			.version = VERSION,
			.author = "Oskari Rauta",
			.copyright = "2026, Oskari Rauta"
		},
		.options = {
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

	} else if ( usage.args.empty()) {

		std::cout << usage << "\n\n" << usage.name() <<
			" needs some arguments, none was provided;\n  try calling it with --help argument\n" << std::endl;
		return 1;
	}

	std::cout << usage << "\n" << std::endl;

	if ( !usage.validated ) {

		auto errors = usage.errors();

		std::cout << "command-line errors found:\n" << errors << std::endl;

		if ( auto it = std::find_if(errors.begin(), errors.end(),
			[](usage_t::error_t e) {
				return e.error != usage_t::error_type::DUPLICATE && e.error != usage_t::error_type::UNKNOWN_OPTION;
			}); it != errors.end()) {

			std::cout << "\naborting, fatal errors occurred while parsing command-line arguments." << std::endl;
			return 1;
		}
	}

	return 0;
}
