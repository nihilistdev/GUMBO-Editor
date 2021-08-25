#include "IO.h"

int line_nums = 0;
static int quit_times = QUIT_TIMES;

IO::~IO()
{
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);
	Logger::append_log(Logger::time_now(), (char*)"Exit success");
	exit(0);
}

int IO::editor_read_key()
{
	int nread;
	char c;
	while ((nread = (int)read(STDIN_FILENO, &c, 1)) != 1)
	{
		if (nread == -1 && errno != EAGAIN) Error::die("read");
	}
	if (c == '\x1b')
	{
		char seq[3];
		if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
		if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
		if (seq[0] == '[')
		{
			if (seq[1] >= '0' && seq[1] <= '9')
			{
				if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
				if (seq[2] == '~')
				{
					switch (seq[1])
					{
					case '1':
						return (int)EditorKey::HOME_KEY;
					case '2':
						return (int)EditorKey::DEL_KEY;
					case '4':
						return (int)EditorKey::END_KEY;
					case '5':
						return (int)EditorKey::PAGE_UP;
					case '6':
						return (int)EditorKey::PAGE_DOWN;
					case '7':
						return (int)EditorKey::HOME_KEY;
					case '8':
						return (int)EditorKey::END_KEY;
					}
				}
			}
			else
			{
				switch (seq[1])
				{
				case 'A':
					return (int)EditorKey::ARROW_UP;
				case 'B':
					return (int)EditorKey::ARROW_DOWN;
				case 'C':
					return (int)EditorKey::ARROW_RIGHT;
				case 'D':
					return (int)EditorKey::ARROW_LEFT;
				}
			}
		}
		else if (seq[0] == 'O')
		{
			switch (seq[1])
			{
			case 'H':
				return (int)EditorKey::HOME_KEY;
			case 'F':
				return (int)EditorKey::END_KEY;
			}
		}
		return '\x1b';
	}
	else
	{
		return c;
	}
}

void IO::toogle_line_nums()
{
	switch (line_nums)
	{
	case 1:
		line_nums = 0;
		break;
	case 0:
		line_nums = 1;
		break;
	}
}

void IO::editor_process_keypress()
{
	std::string value;
	int c = editor_read_key();
	switch (c)
	{
	case '\r':
		editor_insert_new_line();
		break;
	case CTRL_KEY('q'):
		if (e.dirty && quit_times > 0)
		{
			RawMode::editor_set_status_message("WARNING!!! File has unsaved changes. "
											   "Press Ctrl-Q %d more times to quit.", quit_times);
			quit_times--;
			return;
		}
		IO();
		break;

	case CTRL_KEY('h'):
		RawMode::editor_set_status_message("HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-N = Line numbers");
		break;

	case CTRL_KEY('n'):
		toogle_line_nums();
		break;

	case CTRL_KEY('s'):
		File::save();
		break;

	case (int)EditorKey::HOME_KEY:
		e.cx = 0;
		break;
	case (int)EditorKey::END_KEY:
		if (e.cy < e.num_rows)
			e.cx = e.row[e.cy].size;

	case (int)EditorKey::BACKSPACE:
	case (int)EditorKey::DEL_KEY:
		if (c == (int)EditorKey::DEL_KEY) Window::move_cursor((int)EditorKey::ARROW_RIGHT);
		editor_delete_char();
		break;

	case (int)EditorKey::PAGE_UP:
	case (int)EditorKey::PAGE_DOWN:
	{
		if (c == (int)EditorKey::PAGE_UP)
		{
			e.cy = e.rowoff;
		}
		else if (c == (int)EditorKey::PAGE_DOWN)
		{
			e.cy = e.rowoff + e.screenrows - 1;
			if (e.cy > e.num_rows) e.cy = e.num_rows;
		}

		int times = e.screenrows;
		while (times--)
			Window::move_cursor(c == (int)EditorKey::PAGE_UP ? (int)EditorKey::ARROW_UP : (int)EditorKey::ARROW_DOWN);
		break;
	}

	case (int)EditorKey::ARROW_UP:
	case (int)EditorKey::ARROW_DOWN:
	case (int)EditorKey::ARROW_LEFT:
	case (int)EditorKey::ARROW_RIGHT:
		Window::move_cursor(c);
		break;

	case CTRL_KEY('l'):
	case '\x1b':
		break;

	case CTRL_KEY('f'):
		File::search();
		break;

	case CTRL_KEY('c'):
		clip::clear();
		clip::set_text(e.row[e.cy].chars);
		clip::get_text(value);
		RawMode::editor_set_status_message("Line copied!");
		break;

	case CTRL_KEY('v'):
		clip::get_text(value);
		Row::append_string(&e.row[e.cy],
			strcpy(new char[value.length() + 1], value.c_str()),
			strlen(strcpy(new char[value.length() + 1], value.c_str())));
		break;

	default:
		editor_insert_char(c);
		break;
	}
	quit_times = QUIT_TIMES;
}

void IO::editor_refresh_screen()
{
	editor_scroll();

	struct InputBuffer inputBuffer = INPUT_BUFFER_INIT;

	append(&inputBuffer, "\x1b[?25l", 6);
	append(&inputBuffer, "\x1b[H", 3);

	editor_draw_rows(&inputBuffer);
	editor_draw_status_bar(&inputBuffer);
	draw_message_bar(&inputBuffer);

	char buffer[32];
	if (line_nums == 1)
	{
		snprintf(buffer,
			sizeof(buffer),
			"\x1b[%d;%dH",
			(e.cy - e.rowoff) + 1,
			(e.rx - e.coloff) + 6);
	}
	else if (line_nums == 0)
	{
		snprintf(buffer,
			sizeof(buffer),
			"\x1b[%d;%dH",
			(e.cy - e.rowoff) + 1,
			(e.rx - e.coloff) + 1);
	}
	append(&inputBuffer, buffer, (int)strlen(buffer));

	append(&inputBuffer, "\x1b[?25h", 6);

	write(STDOUT_FILENO, inputBuffer.b, inputBuffer.len);
	free_input_buffer(&inputBuffer);
}

void IO::line_numbers(struct InputBuffer* inputBuffer, int line)
{
	char num[10];
	//for now
	if (line < 10)
	{
		int numlen = snprintf(num, sizeof(num), "%d    ", line);
		append(inputBuffer, num, numlen);
	}
	else if (line >= 10)
	{
		int numlen = snprintf(num, sizeof(num), "%d   ", line);
		append(inputBuffer, num, numlen);
	}
	else if (line >= 100)
	{
		int numlen = snprintf(num, sizeof(num), "%d", line);
		if (numlen > e.screencols) numlen = e.screencols;
		append(inputBuffer, num, numlen);
		int padding = (e.screencols - numlen) / 48;
		if (padding)
		{
			padding--;
		}
		while (padding--) append(inputBuffer, " ", 1);
	}
}

void IO::editor_draw_rows(struct InputBuffer* inputBuffer)
{
	int y;
	for (y = 0; y < e.screenrows; y++)
	{
		int filerow = y + e.rowoff;
		if (line_nums == 1) line_numbers(inputBuffer, filerow + 1);
		if (filerow >= e.num_rows)
		{
			if (e.num_rows == 0 && y == e.screenrows / 3)
			{
				char welcome[80];
				int welcomelen = snprintf(welcome, sizeof(welcome),
					"GUMBO -- version %s", VERSION);
				if (welcomelen > e.screencols) welcomelen = e.screencols;
				int padding = (e.screencols - welcomelen) / 2;
				if (padding)
				{
					append(inputBuffer, "~", 1);
					padding--;
				}
				while (padding--) append(inputBuffer, " ", 1);
				append(inputBuffer, welcome, welcomelen);
			}
			else
			{
				append(inputBuffer, "~", 1);
			}
		}
		else
		{
			int len = e.row[filerow].rsize - e.coloff;
			if (len < 0) len = 0;
			if (len > e.screencols) len = e.screencols;
			char* c = &e.row[filerow].render[e.coloff];
			unsigned char* hl = &e.row[filerow].hl[e.coloff];
			int current_color = -1;
			int j;
			for (j = 0; j < len; j++)
			{
				if (iscntrl(c[j]))
				{
					char sym = (c[j] <= 26) ? '@' + c[j] : '?';
					append(inputBuffer, "\x1b[7m", 4);
					append(inputBuffer, &sym, 1);
					append(inputBuffer, "\x1b[m", 3);
					if (current_color != -1)
					{
						char buf[16];
						int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
						append(inputBuffer, buf, clen);
					}
				}
				else if (hl[j] == (int)EditorHighlight::HL_NORMAL)
				{
					if (current_color != -1)
					{
						append(inputBuffer, "\x1b[39m", 5);
						current_color = -1;
					}
					append(inputBuffer, &c[j], 1);
				}
				else
				{
					int color = Syntax::editor_syntax_color(hl[j]);
					if (color != current_color)
					{
						current_color = color;
						char buf[16];
						int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
						append(inputBuffer, buf, clen);
					}
					append(inputBuffer, &c[j], 1);
				}
			}
			append(inputBuffer, "\x1b[39m", 5);
		}
		append(inputBuffer, "\x1b[K", 3);
		append(inputBuffer, "\r\n", 2);
	}
}

void IO::append(struct InputBuffer* inputBuffer, const char* s, int len)
{
	char* new_line = (char*)realloc(inputBuffer->b, inputBuffer->len + len);

	if (new_line == nullptr) return;

	memcpy(&new_line[inputBuffer->len], s, len);
	inputBuffer->b = new_line;
	inputBuffer->len += len;
}

void IO::free_input_buffer(struct InputBuffer* inputBuffer)
{
	free(inputBuffer->b);
}

void IO::editor_scroll()
{
	e.rx = 0;
	if (e.cy < e.num_rows)
	{
		e.rx = Row::editor_row_cx_to_rx(&e.row[e.cy], e.cx);
	}

	if (e.cy < e.rowoff)
	{
		e.rowoff = e.cy;
	}
	if (e.cy >= e.rowoff + e.screenrows)
	{
		e.rowoff = e.cy - e.screenrows + 1;
	}
	if (e.rx < e.coloff)
	{
		e.coloff = e.rx;
	}
	if (e.rx >= e.coloff + e.screencols)
	{
		e.coloff = e.rx - e.screencols + 1;
	}
}

void IO::editor_draw_status_bar(struct InputBuffer* inputBuffer)
{
	append(inputBuffer, "\x1b[7m", 4);
	char status[80], rstatus[80], lines[80];
	int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
		e.filename ? e.filename : "[No Name]", e.num_rows,
		e.dirty ? "(modified)" : "");
	int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d",
		e.syntax ? e.syntax->filetype : "no ft", e.cy + 1, e.num_rows);
	if (len > e.screencols) len = e.screencols;
	append(inputBuffer, status, len);
	while (len < e.screencols)
	{
		if (e.screencols - len == rlen)
		{
			append(inputBuffer, rstatus, rlen);
			break;
		}
		else
		{
			append(inputBuffer, (char*)" ", 1);
			len++;
		}
	}
	append(inputBuffer, "\x1b[m", 3);
	append(inputBuffer, "\r\n", 2);
}

void IO::draw_message_bar(struct InputBuffer* inputBuffer)
{
	append(inputBuffer, "\x1b[K", 3);
	int msglen = strlen(e.statusmsg);
	if (msglen > e.screencols) msglen = e.screencols;
	if (msglen && time(NULL) - e.statusmsg_time < 5)
		append(inputBuffer, e.statusmsg, msglen);
}

void IO::editor_insert_char(int c)
{
	if (e.cy == e.num_rows)
	{
		Row::editor_insert_row(e.num_rows, (char*)"", 0);
	}
	Row::editor_row_insert_char(&e.row[e.cy], e.cx, c);
	e.cx++;
}

void IO::editor_delete_char()
{
	if (e.cy == e.num_rows) return;

	erow* row = &e.row[e.cy];
	if (e.cx > 0)
	{
		Row::editor_row_delete_cahr(row, (e.cx - 1));
		e.cx--;
	}
	else if (e.cy != 0)
	{
		if (e.cx == 0)
		{
			Row::delete_row(e.cy);
		}
		e.cy--;
		Window::move_cursor((int)EditorKey::ARROW_RIGHT);
	}
}

void IO::editor_insert_new_line()
{
	if (e.cx == 0)
	{
		Row::editor_insert_row(e.cy, (char*)"", 0);
	}
	else
	{
		erow* row = &e.row[e.cy];
		Row::editor_insert_row(e.cy + 1, &row->chars[e.cx], row->size - e.cx);
		row = &e.row[e.cy];
		row->size = e.cx;
		row->chars[row->size] = '\0';
		Row::update_row(row);
	}
	e.cy++;
	e.cx = 0;
}

char* IO::editor_prompt(char* prompt, void (* callback)(char*, int))
{
	size_t bufsize = 128;
	char* buf = (char*)malloc(bufsize);
	size_t buflen = 0;
	buf[0] = '\0';
	while (1)
	{
		RawMode::editor_set_status_message(prompt, buf);
		editor_refresh_screen();
		int c = editor_read_key();
		if (c == (int)EditorKey::DEL_KEY || c == CTRL_KEY('h') || c == (int)EditorKey::BACKSPACE)
		{
			if (buflen != 0) buf[--buflen] = '\0';
		}
		else if (c == '\x1b')
		{
			RawMode::editor_set_status_message("");
			if (callback) callback(buf, c);
			free(buf);
			return NULL;
		}
		else if (c == '\r')
		{
			if (buflen != 0)
			{
				RawMode::editor_set_status_message((char*)"");
				if (callback) callback(buf, c);
				return buf;
			}
		}
		else if (!iscntrl(c) && c < 128)
		{
			if (buflen == bufsize - 1)
			{
				bufsize *= 2;
				buf = (char*)realloc(buf, bufsize);
			}
			buf[buflen++] = c;
			buf[buflen] = '\0';
		}
		if (callback) callback(buf, c);
	}
}