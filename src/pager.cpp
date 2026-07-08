#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <poll.h>
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
	}

	void run_pager(const std::vector<std::string>& lines, const std::string& title) {

		if ( !isatty(STDIN_FILENO)) {
			// no keyboard - fall back to plain dump
			for ( auto& l : lines ) { std::fputs(l.c_str(), stdout); std::fputc('\n', stdout); }
			return;
		}

		raw_mode();

		int top = 0;
		const int total = (int)lines.size();
		bool quit = false;

		while ( !quit ) {

			int rows, cols;
			term_size(rows, cols);
			int height = rows - 1;
			if ( height < 1 ) height = 1;
			int max_top = std::max(0, total - height);
			if ( top > max_top ) top = max_top;
			if ( top < 0 ) top = 0;

			std::string buf = "\033[H"; // cursor home

			for ( int r = 0; r < height; r++ ) {
				int idx = top + r;
				buf += "\033[K"; // clear line
				if ( idx < total )
					buf += clip_visible(lines[idx], (size_t)cols);
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

			char st[512];
			std::snprintf(st, sizeof(st), " %s   line %d/%d   %s   [\u2191\u2193 PgUp/Dn Space=page b=back g/G q=quit] ",
				title.c_str(), bottom, total, pos.c_str());
			std::string status = st;
			status = clip_visible(status, (size_t)cols);
			// pad to full width
			size_t vw = visible_width(status);
			if ( vw < (size_t)cols ) status += std::string((size_t)cols - vw, ' ');

			buf += "\033[K" + ansi::reverse + status + ansi::reset;

			ssize_t w = write(STDOUT_FILENO, buf.c_str(), buf.size()); (void)w;

			int k = read_key();
			switch ( k ) {
				case K_UP:            top -= 1; break;
				case K_DOWN:          top += 1; break;
				case K_PGUP:          top -= height; break;
				case K_PGDN: case ' ': top += height; break;
				case 'b': case 'B':   top -= height; break;
				case K_HOME: case 'g': top = 0; break;
				case K_END:  case 'G': top = max_top; break;
				case 'q': case 'Q': case K_ESC: quit = true; break;
				case K_ENTER:         top += 1; break;
				default: break;
			}
		}

		restore_term();
	}

} // namespace mcat
