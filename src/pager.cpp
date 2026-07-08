#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <cctype>
#include <cstdio>
#include <string>
#include <algorithm>
#include "ansi.hpp"
#include "mcat.hpp"

namespace mcat {

	namespace {

		enum key : int {
			K_NONE = 0, K_UP = 1000, K_DOWN, K_LEFT, K_RIGHT,
			K_PGUP, K_PGDN, K_HOME, K_END, K_ESC, K_ENTER
		};

		struct termios saved_tio;

		void restore_term() {
			tcsetattr(STDIN_FILENO, TCSANOW, &saved_tio);
			// leave alternate screen, show cursor
			const char* seq = "\033[?25h\033[?1049l";
			ssize_t r = write(STDOUT_FILENO, seq, 14); (void)r;
		}

		void raw_mode() {
			tcgetattr(STDIN_FILENO, &saved_tio);
			struct termios t = saved_tio;
			t.c_lflag &= ~( ICANON | ECHO );
			t.c_iflag &= ~( IXON | ICRNL );
			t.c_cc[VMIN] = 1;
			t.c_cc[VTIME] = 0;
			tcsetattr(STDIN_FILENO, TCSANOW, &t);
			// enter alternate screen, hide cursor
			const char* seq = "\033[?1049h\033[?25l";
			ssize_t r = write(STDOUT_FILENO, seq, 14); (void)r;
		}

		void term_size(int& rows, int& cols) {
			struct winsize ws;
			if ( ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 ) {
				rows = ws.ws_row;
				cols = ws.ws_col;
			} else {
				rows = 24;
				cols = 80;
			}
		}

		// read a single logical key, decoding escape sequences
		int read_key() {

			unsigned char c;
			if ( read(STDIN_FILENO, &c, 1) != 1 ) return K_NONE;

			if ( c == '\r' || c == '\n' ) return K_ENTER;
			if ( c != 27 ) return c; // ordinary byte

			// got ESC: is a sequence following?
			struct pollfd pfd = { STDIN_FILENO, POLLIN, 0 };
			if ( poll(&pfd, 1, 40) <= 0 ) return K_ESC; // lone ESC

			unsigned char b1;
			if ( read(STDIN_FILENO, &b1, 1) != 1 ) return K_ESC;
			if ( b1 != '[' && b1 != 'O' ) return K_ESC;

			unsigned char b2;
			if ( read(STDIN_FILENO, &b2, 1) != 1 ) return K_ESC;

			switch ( b2 ) {
				case 'A': return K_UP;
				case 'B': return K_DOWN;
				case 'C': return K_RIGHT;
				case 'D': return K_LEFT;
				case 'H': return K_HOME;
				case 'F': return K_END;
			}

			if ( b2 >= '0' && b2 <= '9' ) {
				unsigned char b3;
				if ( read(STDIN_FILENO, &b3, 1) != 1 ) return K_ESC; // consume '~'
				switch ( b2 ) {
					case '1': case '7': return K_HOME;
					case '4': case '8': return K_END;
					case '5': return K_PGUP;
					case '6': return K_PGDN;
				}
			}
			return K_NONE;
		}

		// Read a line of text at the bottom row (for the search prompt).
		// Returns false if the user cancelled with ESC.
		bool prompt_line(int row, int cols, const std::string& prefix, std::string& result) {

			result.clear();
			const char* show = "\033[?25h";
			const char* hide = "\033[?25l";

			while ( true ) {

				std::string shown = clip_visible(prefix + result, (size_t)cols);
				std::string buf = "\033[" + std::to_string(row) + ";1H\033[K" +
					ansi::reverse + shown + ansi::reset + show;
				ssize_t w = write(STDOUT_FILENO, buf.c_str(), buf.size()); (void)w;

				unsigned char c;
				if ( read(STDIN_FILENO, &c, 1) != 1 ) { w = write(STDOUT_FILENO, hide, 6); return false; }

				if ( c == '\r' || c == '\n' ) { w = write(STDOUT_FILENO, hide, 6); return true; }
				if ( c == 27 ) { w = write(STDOUT_FILENO, hide, 6); return false; }
				if ( c == 127 || c == 8 ) { if ( !result.empty()) result.pop_back(); continue; }
				if ( c >= 32 && c < 127 ) result += (char)c;
			}
		}
	}

	void run_pager(const std::vector<std::string>& lines, const std::string& title) {

		if ( !isatty(STDIN_FILENO)) {
			// no keyboard - fall back to plain dump
			for ( auto& l : lines ) { std::fputs(l.c_str(), stdout); std::fputc('\n', stdout); }
			return;
		}

		raw_mode();

		int top = 0;
		int hoff = 0;                 // horizontal scroll offset (columns)
		const int total = (int)lines.size();
		bool quit = false;
		std::string query;            // current search term (lowercased)
		std::string msg;              // transient status message

		// widest visible line (for clamping horizontal scroll), computed once
		size_t maxw = 0;
		for ( auto& l : lines ) { size_t w = visible_width(l); if ( w > maxw ) maxw = w; }

		auto lc = [](std::string s) {
			for ( auto& ch : s ) ch = (char)std::tolower((unsigned char)ch);
			return s;
		};
		auto matches = [&](int idx, const std::string& q) {
			return lc(plain_text(lines[idx])).find(q) != std::string::npos;
		};
		// search from `start`, stepping by `dir` (+1/-1), wrapping around
		auto find = [&](int start, int dir, const std::string& q) -> int {
			if ( q.empty() || total == 0 ) return -1;
			for ( int s = 0; s < total; s++ ) {
				int idx = ((( start + dir * s ) % total ) + total ) % total;
				if ( matches(idx, q)) return idx;
			}
			return -1;
		};

		while ( !quit ) {

			int rows, cols;
			term_size(rows, cols);
			int height = rows - 1;
			if ( height < 1 ) height = 1;
			int max_top = std::max(0, total - height);
			if ( top > max_top ) top = max_top;
			if ( top < 0 ) top = 0;
			int max_hoff = (int)maxw > cols ? (int)maxw - 1 : 0;
			if ( hoff > max_hoff ) hoff = max_hoff;
			if ( hoff < 0 ) hoff = 0;

			std::string buf = "\033[H"; // cursor home

			for ( int r = 0; r < height; r++ ) {
				int idx = top + r;
				buf += "\033[K"; // clear line
				if ( idx < total ) {
					const std::string& raw = lines[idx];
					std::string shown = query.empty() ? raw : highlight_matches(raw, query);
					buf += clip_visible_range(shown, (size_t)hoff, (size_t)cols);
				}
				buf += "\r\n";
			}

			// status line
			int bottom = std::min(top + height, total);
			int pct = total <= 0 ? 100 : (int)((long)bottom * 100 / total);
			std::string pos;
			if ( total <= height ) pos = "ALL";
			else if ( top == 0 ) pos = "TOP";
			else if ( bottom >= total ) pos = "END";
			else { char b[8]; std::snprintf(b, sizeof(b), "%d%%", pct); pos = b; }

			std::string extra;
			if ( hoff > 0 ) extra += "  >" + std::to_string(hoff);
			if ( !msg.empty()) extra += "  " + msg;
			else if ( !query.empty()) extra += "  /" + query;

			char st[512];
			std::snprintf(st, sizeof(st), " %s   line %d/%d   %s%s   [\u2191\u2193\u2190\u2192 Space/b /=search n/N g/G q] ",
				title.c_str(), bottom, total, pos.c_str(), extra.c_str());
			std::string status = clip_visible(st, (size_t)cols);
			size_t vw = visible_width(status);
			if ( vw < (size_t)cols ) status += std::string((size_t)cols - vw, ' ');

			buf += "\033[K" + ansi::reverse + status + ansi::reset;

			ssize_t w = write(STDOUT_FILENO, buf.c_str(), buf.size()); (void)w;

			msg.clear();
			int k = read_key();
			switch ( k ) {
				case K_UP:             top -= 1; break;
				case K_DOWN:           top += 1; break;
				case K_PGUP:           top -= height; break;
				case K_PGDN: case ' ': top += height; break;
				case 'b': case 'B':    top -= height; break;
				case K_LEFT:           hoff -= 8; break;
				case K_RIGHT:          hoff += 8; break;
				case '0':              hoff = 0; break;
				case K_HOME: case 'g': top = 0; break;
				case K_END:  case 'G': top = max_top; break;
				case '/': {
					std::string q;
					if ( prompt_line(rows, cols, "/", q) && !q.empty()) {
						query = lc(q);
						int m = find(top, 1, query);
						if ( m < 0 ) msg = "not found";
						else top = m;
					}
					break;
				}
				case 'n': {
					if ( !query.empty()) {
						int m = find(top + 1, 1, query);
						if ( m < 0 ) msg = "not found"; else top = m;
					}
					break;
				}
				case 'N': {
					if ( !query.empty()) {
						int m = find(top - 1, -1, query);
						if ( m < 0 ) msg = "not found"; else top = m;
					}
					break;
				}
				case 'q': case 'Q': case K_ESC: quit = true; break;
				case K_ENTER:          top += 1; break;
				default: break;
			}
		}

		restore_term();
	}

} // namespace mcat
