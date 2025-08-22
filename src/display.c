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
static inline u8	_pad(size_t n);

static inline void	_calculate_padding(size_t *left, size_t *right, const size_t longest_title, const size_t len);
static inline void	_calculate_top_left_xy(u32 *pos[2], const u16 block_width, const u16 block_height);

static inline void	_update_window_size([[gnu::unused]] i32 sig);

u8	display_menu(const menu *menu) {
	const menu_item	*current;
	const menu_item	*down;
	struct {
		size_t	left;
		size_t	right;
	}				padding;
	u32				root_x;
	u32				root_y;
	u16				menu_width;
	u16				menu_height;
	u16				max_visible_x;
	u16				max_visible_y;


	menu_width = menu->width * (menu->longest_title + 2);
	menu_height = menu->height;
	if ((menu_width * 100) / window_size.width.cells > 100 - DISPLAY_MIN_MARGIN * 2) {
		for (max_visible_x = menu->width - 1; max_visible_x; max_visible_x--)
			if (((max_visible_x * (menu->longest_title + 2)) * 100) / window_size.width.cells < 100 - DISPLAY_MIN_MARGIN * 2)
				break ;
	} else
		max_visible_x = menu->width;
	if ((menu_height * 100) / window_size.height.cells > 100 - DISPLAY_MIN_MARGIN * 2) {
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
		_calculate_top_left_xy((u32*[2]){&root_x, &root_y}, menu_width, menu_height);
		if (fprintf(stdout, "\x1b[%hu;%huH\x1b[2J", root_y++, root_x) == -1)
			return 0;
		for (current = menu->root; current; current = down) {
			for (down = current->neighbors.down; current; current = current->neighbors.right) {
				_calculate_padding(&padding.left, &padding.right, menu->longest_title, strlen(current->title));
				if (!_pad(padding.left))
					return 0;
				if (current->selected) {
					if (set_color_fg(colors.selection.fg) == -1 || set_color_bg(colors.selection.bg) == -1)
						return 0;
				} else if (set_color_fg(colors.fg) == -1)
					return 0;
				if (fprintf(stdout, " %s \x1b[m", current->title) == -1)
					return 0;
				if (!_pad(padding.right))
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
	const menu_item	*down;
	const menu_item	*start;
	const menu_item	*current;
	struct {
		size_t	left;
		size_t	right;
	}				padding;
	size_t			longest_title;
	size_t			i;
	size_t			j;
	u32				root_x;
	u32				root_y;
	u16				menu_width;
	u16				menu_height;
	u16				selection_row;
	u16				selection_col;
	u16				start_row;
	u16				start_col;

	for (selection_row = 1, current = menu->root; current; current = down, selection_row++) {
		for (selection_col = 1, down =current->neighbors.down; current; current = current->neighbors.right, selection_col++)
			if (current->selected)
				break ;
		if (current)
			break ;
		current = down;
	}
	if (selection_row <= max_visible_y / 2)
		start_row = 1;
	else if (selection_row > menu->height - max_visible_y / 2)
		start_row = menu->height - max_visible_y + 1;
	else
		start_row = selection_row - max_visible_y / 2;
	if (selection_col <= max_visible_x / 2)
		start_col = 1;
	else if (selection_col > menu->width - max_visible_x / 2)
		start_col = menu->width - max_visible_x + 1;
	else
		start_col = selection_col - max_visible_x / 2;
	for (current = menu->root, i = 1; i < start_row; i++)
		current = current->neighbors.down;
	for (i = 1; i < start_col; i++)
		current = current->neighbors.right;
	for (start = current, longest_title = i = 0; i < max_visible_y; i++) {
		for (down = current->neighbors.down, j = 0; j < max_visible_x; j++) {
			if (longest_title < strlen(current->title))
				longest_title = strlen(current->title);
			current = current->neighbors.right;
		}
		current = down;
	}
	menu_width = max_visible_x * (longest_title + 2);
	menu_height = max_visible_y;
	_calculate_top_left_xy((u32*[2]){&root_x, &root_y}, menu_width, menu_height);
	if (fprintf(stdout, "\x1b[%hu;%huH\x1b[2J", root_y++, root_x) == -1)
		return 0;
	for (current = start, i = 0; i < max_visible_y; i++) {
		for (down = current->neighbors.down, j = 0; j < max_visible_x; j++) {
			_calculate_padding(&padding.left, &padding.right, longest_title, strlen(current->title));
			if (!_pad(padding.left))
				return 0;
			if (current->selected) {
				if (set_color_fg(colors.selection.fg) == -1 || set_color_bg(colors.selection.bg) == -1)
					return 0;
			} else if (set_color_fg(colors.fg) == -1)
				return 0;
			if (fprintf(stdout, " %s \x1b[m", current->title) == -1)
				return 0;
			if (!_pad(padding.right))
				return 0;
			current = current->neighbors.right;
		}
		if (fprintf(stdout, "\x1b[%hu;%huH", root_y++, root_x) == -1)
			return 0;
		current = down;
	}
	return fflush(stdout) != EOF;
}

static inline u8	_pad(size_t n) {
	while (n--)
		if (fputc(' ', stdout) == EOF)
			return 0;
	return 1;
}

static inline void	_calculate_padding(size_t *left, size_t *right, const size_t longest_title, const size_t len) {
	size_t	padding_needed;

	padding_needed = longest_title - len;
	*left = padding_needed / 2 + (padding_needed & 1);
	*right = padding_needed / 2;
}

static inline void	_calculate_top_left_xy(u32 *pos[2], const u16 block_width, const u16 block_height) {
	struct {
		u32	top_left_x;
		u32	top_left_y;
		u32	top_right_x;
		u32	bot_left_y;
	}	corners;
	
	corners.top_left_x = window_size.width.cells / 2 - block_width / 2 - 1;
	corners.top_left_y = window_size.height.cells / 2 - block_height / 2 - 1;
	corners.top_right_x = corners.top_left_x + block_width - 1;
	while (corners.top_left_x - 1 > window_size.width.cells - corners.top_right_x) {
		corners.top_right_x--;
		corners.top_left_x--;
	}
	while (corners.top_left_x - 1 < window_size.width.cells - corners.top_right_x) {
		corners.top_right_x++;
		corners.top_left_x++;
	}
	corners.bot_left_y = corners.top_left_y + block_height - 1;
	while (corners.top_left_y - 1 > window_size.height.cells - corners.bot_left_y) {
		corners.bot_left_y--;
		corners.top_left_y--;
	}
	while (corners.top_left_y - 1 < window_size.height.cells - corners.bot_left_y) {
		corners.bot_left_y++;
		corners.top_left_y++;
	}
	*pos[0] = corners.top_left_x;
	*pos[1] = corners.top_left_y;
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
