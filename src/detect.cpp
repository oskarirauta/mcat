#include <algorithm>
#include <cctype>
#include <cstdio>
#include "mcat.hpp"

namespace mcat {

	static std::string lower(std::string s) {
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char c){ return std::tolower(c); });
		return s;
	}

	static std::string extension(const std::string& path) {
		size_t slash = path.find_last_of("/\\");
		std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
		size_t dot = name.find_last_of('.');
		if ( dot == std::string::npos || dot == 0 ) return "";
		return lower(name.substr(dot + 1));
	}

	static std::string basename_lower(const std::string& path) {
		size_t slash = path.find_last_of("/\\");
		std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
		return lower(name);
	}

	bool is_binary(const std::string& data) {

		if ( data.empty()) return false;

		// UTF BOMs -> text
		if ( data.size() >= 3 && (unsigned char)data[0] == 0xEF &&
			(unsigned char)data[1] == 0xBB && (unsigned char)data[2] == 0xBF ) return false;

		size_t limit = std::min<size_t>(data.size(), 8000);
		size_t suspicious = 0;

		for ( size_t i = 0; i < limit; i++ ) {
			unsigned char c = (unsigned char)data[i];
			if ( c == 0 ) return true; // NUL byte -> definitely binary
			// allow common control chars: tab, lf, cr, ff, esc, bs
			if ( c == '\t' || c == '\n' || c == '\r' || c == '\f' ||
				c == '\b' || c == 0x1b ) continue;
			if ( c < 0x20 ) suspicious++;
		}

		// >2% non-text control bytes -> treat as binary
		return suspicious * 100 > limit * 2;
	}

	filetype type_from_name(const std::string& path) {

		std::string base = basename_lower(path);
		std::string ext = extension(path);

		// well-known filenames without a useful extension
		static const char* cfg_names[] = {
			"makefile", "dockerfile", ".gitconfig", ".gitmodules",
			".gitignore", ".env", "cmakelists.txt", nullptr };
		for ( int i = 0; cfg_names[i]; i++ )
			if ( base == cfg_names[i] ) return filetype::CONFIG;

		if ( ext == "json" ) return filetype::JSON;
		if ( ext == "md" || ext == "markdown" ) return filetype::MARKDOWN;

		if ( ext == "c" || ext == "h" || ext == "cpp" || ext == "cc" ||
			ext == "cxx" || ext == "hpp" || ext == "hxx" || ext == "hh" ||
			ext == "inc" || ext == "ino" ) return filetype::CPP;

		if ( ext == "ini" || ext == "conf" || ext == "cfg" || ext == "config" ||
			ext == "toml" || ext == "yml" || ext == "yaml" || ext == "properties" ||
			ext == "desktop" || ext == "service" || ext == "rc" || ext == "env" )
			return filetype::CONFIG;

		if ( ext == "txt" || ext == "text" || ext == "log" ) return filetype::PLAIN;

		return filetype::UNKNOWN;
	}

	// light content sniff when the extension gave nothing away
	static filetype sniff_content(const std::string& data) {

		// skip leading whitespace
		size_t i = 0;
		while ( i < data.size() && std::isspace((unsigned char)data[i])) i++;
		if ( i >= data.size()) return filetype::PLAIN;

		char c = data[i];
		if ( c == '{' || c == '[' ) return filetype::JSON;
		if ( c == '#' ) {
			// could be markdown heading or a shell/config comment; look for "# "
			if ( i + 1 < data.size() && ( data[i+1] == ' ' || data[i+1] == '#' ))
				return filetype::MARKDOWN;
			return filetype::CONFIG;
		}
		return filetype::PLAIN;
	}

	filetype detect_type(const std::string& path, const std::string& data) {

		if ( is_binary(data)) return filetype::BINARY;

		filetype t = type_from_name(path);
		if ( t != filetype::UNKNOWN ) return t;

		return sniff_content(data);
	}

	// ---- binary info ----------------------------------------------------

	static std::string human_size(size_t n) {
		const char* u[] = { "B", "KiB", "MiB", "GiB", "TiB" };
		double v = (double)n;
		int i = 0;
		while ( v >= 1024.0 && i < 4 ) { v /= 1024.0; i++; }
		char buf[64];
		if ( i == 0 ) std::snprintf(buf, sizeof(buf), "%zu %s", n, u[i]);
		else std::snprintf(buf, sizeof(buf), "%.1f %s (%zu bytes)", v, u[i], n);
		return buf;
	}

	// tiny magic table - "enough" common formats
	static std::string magic_guess(const std::string& d) {

		auto has = [&](const char* sig, size_t len, size_t off = 0) {
			if ( d.size() < off + len ) return false;
			return std::equal(sig, sig + len, d.begin() + off);
		};

		// executables / objects
		if ( has("\x7f""ELF", 4)) return "ELF executable / object";
		if ( has("\xca\xfe\xba\xbe", 4)) return "Java class file";
		if ( has("\x00""asm", 4)) return "WebAssembly (wasm) binary";
		if ( has("MZ", 2)) return "DOS/Windows executable";

		// images
		if ( has("\x89PNG", 4)) return "PNG image";
		if ( has("\xff\xd8\xff", 3)) return "JPEG image";
		if ( has("GIF87a", 6) || has("GIF89a", 6)) return "GIF image";
		if ( has("BM", 2)) return "BMP image";
		if ( has("\x00\x00\x01\x00", 4)) return "icon (ICO)";
		if ( has("II*\x00", 4) || has("MM\x00*", 4)) return "TIFF image";

		// RIFF container - distinguish by the form type at offset 8
		if ( has("RIFF", 4)) {
			if ( has("WEBP", 4, 8)) return "WebP image";
			if ( has("WAVE", 4, 8)) return "WAV audio";
			if ( has("AVI ", 4, 8)) return "AVI video";
			return "RIFF container";
		}

		// ISO base media (mp4/mov/heic...) - 'ftyp' box at offset 4
		if ( has("ftyp", 4, 4)) {
			if ( has("heic", 4, 8) || has("heif", 4, 8) || has("mif1", 4, 8)) return "HEIF/HEIC image";
			if ( has("qt  ", 4, 8)) return "QuickTime video";
			return "MP4 / ISO media";
		}

		// fonts
		if ( has("wOFF", 4)) return "WOFF font";
		if ( has("wOF2", 4)) return "WOFF2 font";
		if ( has("\x00\x01\x00\x00", 4)) return "TrueType font";
		if ( has("OTTO", 4)) return "OpenType font";

		// audio
		if ( has("OggS", 4)) return "Ogg media";
		if ( has("fLaC", 4)) return "FLAC audio";
		if ( has("ID3", 3)) return "MP3 audio (ID3)";

		// documents
		if ( has("%PDF", 4)) return "PDF document";

		// archives / compression
		if ( has("PK\x03\x04", 4) || has("PK\x05\x06", 4)) return "ZIP archive (or zip-based: jar/docx/...)";
		if ( has("Rar!\x1a\x07", 6)) return "RAR archive";
		if ( has("7z\xbc\xaf\x27\x1c", 6)) return "7-zip archive";
		if ( has("\x1f\x8b", 2)) return "gzip compressed data";
		if ( has("BZh", 3)) return "bzip2 compressed data";
		if ( has("\xfd""7zXZ", 6)) return "xz compressed data";
		if ( has("\x28\xb5\x2f\xfd", 4)) return "zstandard compressed data";
		if ( has("\x04\x22\x4d\x18", 4)) return "lz4 compressed data";
		if ( has("ustar", 5, 257)) return "tar archive";

		// databases
		if ( has("SQLite format 3\x00", 16)) return "SQLite 3 database";

		return "";
	}

	std::string binary_info(const std::string& path, const std::string& data) {

		std::string kind = magic_guess(data);
		std::string out = "binary file";
		if ( !kind.empty()) out += ": " + kind;
		out += "\n  size: " + human_size(data.size());

		if ( kind.empty() && !data.empty()) {
			// show first bytes as hex to give a hint
			out += "\n  magic:";
			size_t n = std::min<size_t>(data.size(), 8);
			char buf[8];
			for ( size_t i = 0; i < n; i++ ) {
				std::snprintf(buf, sizeof(buf), " %02x", (unsigned char)data[i]);
				out += buf;
			}
		}
		return out;
	}

} // namespace mcat
