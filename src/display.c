// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<display.c>>

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>

#include "display.h"

#define set_color_fg(x)	(fprintf(stdout, "\x1b[38;5;%hhum", x))
#define set_color_bg(x)	(fprintf(stdout, "\x1b[48;5;%hhum", x))

struct {
	u8	fg;
	struct {
		u8	fg;
		u8	bg;
	}	selection;
}	colors = {
	.fg= 231,
	.selection.fg = 16,
	.selection.bg = 231
};

struct {
	struct {
		u32	cells;
		u32	px;
	}	width;
	struct {
		u32	cells;
		u32	px;
	}	height;
}	window_size;

static inline u8	_scroll_menu(const menu *menu, const u16 max_visibe_x, const u16 max_visible_y);
static inline u8	_pad(const size_t longest, size_t len);

static inline void	_update_window_size([[gnu::unused]] i32 sig);

u8	display_menu(const menu *menu) {
	const menu_item	*current;
	const menu_item	*down;
	u16				menu_x;
	u16				menu_y;
	u16				root_x;
	u16				root_y;
	u16				max_visible_x;
	u16				max_visible_y;


	menu_x = menu->width * (menu->longest_title + 2);
	menu_y = menu->height;
	if ((menu_x * 100) / window_size.width.cells > 100 - DISPLAY_MIN_MARGIN * 2) {
		for (max_visible_x = menu->width - 1; max_visible_x; max_visible_x--)
			if (((max_visible_x * (menu->longest_title + 2)) * 100) / window_size.width.cells < 100 - DISPLAY_MIN_MARGIN * 2)
				break ;
	} else
		max_visible_x = menu->width;
	if ((menu_y * 100) / window_size.height.cells > 100 - DISPLAY_MIN_MARGIN * 2) {
		for (max_visible_y = menu->height - 1; max_visible_y; max_visible_y--)
			if ((max_visible_y * 100) / window_size.height.cells < 100 - DISPLAY_MIN_MARGIN * 2)
				break ;
	} else
		max_visible_y = menu->height;
	if (!max_visible_x || !max_visible_y) {
		if (fputs("\x1b[H\x1b[J\x1b[1;38;5;196m[WINDOW TOO SMALL]\x1b[m", stdout) == EOF)
			return 0;
	} else if (max_visible_x != menu->width || max_visible_y != menu->height)
		return _scroll_menu(menu, max_visible_x, max_visible_y);
	else {
		root_x = window_size.width.cells / 2 - menu_x / 2 + 1;
		root_y = window_size.height.cells / 2 - menu_y / 2 + 1;
		if (fprintf(stderr, "\x1b[%hu;%huH\x1b[2J", root_y++, root_x) == -1)
			return 0;
		for (current = menu->root; current; current = down) {
			for (down = current->neighbors.down; current; current = current->neighbors.right) {
				if (!_pad(menu->longest_title, strlen(current->title)))
					return 0;
				if (current->selected) {
					if (set_color_fg(colors.selection.fg) == -1 || set_color_bg(colors.selection.bg) == -1)
						return 0;
				} else if (set_color_fg(colors.fg) == -1)
					return 0;
				if (fprintf(stdout, " %s \x1b[m", current->title) == -1)
					return 0;
				if (!_pad(menu->longest_title, strlen(current->title) & ~1))
					return 0;
			}
			if (fprintf(stdout, "\x1b[%hu;%huH", root_y++, root_x) == -1)
				return 0;
		}
	}
	return fflush(stdout) != EOF;
}

u8	init_display(void) {
	struct sigaction	action;

	memset(&action, 0, sizeof(action));;
	action.sa_handler = _update_window_size;
	if (sigaction(SIGWINCH, &action, NULL) == -1)
		return 0;
	_update_window_size(0);
	return 1;
}

static inline u8	_scroll_menu(const menu *menu, const u16 max_visible_x, const u16 max_visible_y) {
	(void)menu;
	(void)max_visible_x;
	(void)max_visible_y;
	return 1;
}

static inline u8	_pad(const size_t longest, size_t len) {
	while (len < longest) {
		if (fputc(' ', stdout) == EOF)
			return 0;
		len += 2;
	}
	return 1;
}

static inline void	_update_window_size([[gnu::unused]] i32 sig) {
	struct winsize	win_size;

	if (ioctl(1, TIOCGWINSZ, &win_size) != -1) {
		window_size.height.cells = win_size.ws_row;
		window_size.width.cells = win_size.ws_col;
		window_size.height.px = win_size.ws_ypixel;
		window_size.width.px = win_size.ws_xpixel;
	}
}
