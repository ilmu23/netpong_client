// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<display.c>>

#include <math.h>
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <sys/ioctl.h>

#include "utils.h"
#include "display.h"

#define set_color_fg(x)	(fprintf(stdout, "\x1b[38;5;%hhum", (u8)x))
#define set_color_bg(x)	(fprintf(stdout, "\x1b[48;5;%hhum", (u8)x))

#define _BOX_SIDE_HORIZONTAL	0x2501U
#define _BOX_SIDE_VERTICAL		0x2503U
#define _BOX_CORNER_TL			0x250FU
#define _BOX_CORNER_TR			0x2513U
#define _BOX_CORNER_BL			0x2517U
#define _BOX_CORNER_BR			0x251BU

#define _PADDLE_1Q_UP	0x28C0U
#define _PADDLE_H_UP	0x28E4U
#define _PADDLE_3Q_UP	0x28F6U
#define _PADDLE_FULL	0x28FFU
#define _PADDLE_3Q_DOWN	0x283FU
#define _PADDLE_H_DOWN	0x281BU
#define _PADDLE_1Q_DOWN	0x2809U

#define _BALL_TL	0x28E0U
#define _BALL_TR	0x28C4U
#define _BALL_BL	0x2819U
#define _BALL_BR	0x280BU

#define _WIN_TOO_SMALL	"\x1b[2J[WINDOW TOO SMALL]"

struct {
	u8	fg;
	struct {
		u8	fg;
		u8	bg;
	}	selection;
	u8	paddle;
	u8	ball;
	u8	edge;
}	colors = {
	.fg= 231,
	.selection.fg = 16,
	.selection.bg = 231,
	.paddle = 231,
	.ball = 231,
	.edge = 231
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

static inline u8	_move_to(const u32 x, const u32 y);
static inline u8	_putc_at(const u32 x, const u32 y, const i16 hl[2], const i32 cp);
static inline u8	_puts_at(const u32 x, const u32 y, const i16 hl[2], const char *s);
static inline u8	_printf_at(const u32 x, const u32 y, const i16 hl[2], const char *fmt, ...);

static inline u8	_draw_box(const u32 root_x, const u32 root_y, const u32 width, const u32 height);

static inline u8	_draw_paddle(const f32 paddle_pos, const u32 root_x, const u32 root_y, const u32 offset);
static inline u8	_draw_ball(const f32 ball_pos[2], const u32 root_x, const u32 root_y);

static inline u8	_scroll_menu(const menu *menu, const u16 max_visibe_x, const u16 max_visible_y);
static inline u8	_pad(size_t n);

static inline void	_calculate_padding(size_t *left, size_t *right, const size_t longest_title, const size_t len);
static inline void	_calculate_top_left_xy(u32 *pos[2], const u16 block_width, const u16 block_height);

static inline void	_update_window_size([[gnu::unused]] i32 sig);

u8	display_game(const game *game) {
	static const u16	width = 40 + 2;
	static const u16	height = 20;
	static u8			too_small_printed = 0;
	u32					root_x;
	u32					root_y;
	u32					score_x;
	u32					score_y;

	if (window_size.width.cells < (u32)width + 2 || window_size.height.cells < (u32)height + 2) {
		if (!too_small_printed)
			too_small_printed = (_puts_at(1, 1, (i16[2]){196, -1}, _WIN_TOO_SMALL) && fflush(stdout) != EOF) ? DISPLAY_GAME_WIN_TOO_SMALL : 0;
		return too_small_printed;
	}
	too_small_printed = 0;
	_calculate_top_left_xy((u32*[2]){&root_x, &root_y}, width, height);
	fputs("\x1b[2J", stdout);
	_draw_box(root_x - 1, root_y - 1, width + 2, height + 2);
	_draw_paddle(height - game->p1_pos, root_x, root_y, 0);
	_draw_paddle(height - game->p2_pos, root_x, root_y, width - 1);
	_draw_ball((f32[2]){game->ball.x, height - game->ball.y}, root_x, root_y);
	score_y = height + 2;
	score_x = (width - 8) / 2;
	_printf_at(root_x + score_x, root_y + score_y, (i16[2]){-1, -1}, "%-3hhu--%3hhu", game->p1_score, game->p2_score);
	return (fflush(stdout) != EOF) ? 1 : 0;
}

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
	if (((menu_width + 2) * 100) / window_size.width.cells > 100 - DISPLAY_MENU_MIN_MARGIN * 2) {
		for (max_visible_x = menu->width - 1; max_visible_x; max_visible_x--)
			if (((max_visible_x * (menu->longest_title + 2)) * 100) / window_size.width.cells < 100 - DISPLAY_MENU_MIN_MARGIN * 2)
				break ;
	} else
		max_visible_x = menu->width;
	if (((menu_height + 2) * 100) / window_size.height.cells > 100 - DISPLAY_MENU_MIN_MARGIN * 2) {
		for (max_visible_y = menu->height - 1; max_visible_y; max_visible_y--)
			if ((max_visible_y * 100) / window_size.height.cells < 100 - DISPLAY_MENU_MIN_MARGIN * 2)
				break ;
	} else
		max_visible_y = menu->height;
	if (!max_visible_x || !max_visible_y) {
		if (!_puts_at(1, 1, (i16[2]){196, -1}, _WIN_TOO_SMALL))
			return 0;
	} else if (max_visible_x != menu->width || max_visible_y != menu->height)
		return _scroll_menu(menu, max_visible_x, max_visible_y);
	else {
		_calculate_top_left_xy((u32*[2]){&root_x, &root_y}, menu_width, menu_height);
		fputs("\x1b[2J", stdout);
		_draw_box(root_x - 1, root_y - 1, menu_width + 2, menu_height + 2);
		if (!_move_to(root_x, root_y++))
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
			if (!_move_to(root_x, root_y++))
				return 0;
		}
	}
	return fflush(stdout) != EOF;
}

u8	display_msg(const char **msg) {
	size_t	i;
	struct {
		size_t	left;
		size_t	right;
	}		padding;
	u32		width;
	u32		height;
	u32		root_x;
	u32		root_y;

	for (width = height = 0; msg[height]; height++) {
		if (strlen(msg[height]) > width)
			width = strlen(msg[height]);
	}
	if (window_size.width.cells < (u32)width + 4 || window_size.height.cells < (u32)height + 2)
		return 1;
	_calculate_top_left_xy((u32*[2]){&root_x, &root_y}, width, height);
	if (!_draw_box(root_x - 1, root_y - 1, width + 4, height + 2))
		return 0;
	if (!_move_to(root_x, root_y++))
		return 0;
	for (i = 0; i < height; i++) {
		_calculate_padding(&padding.left, &padding.right, width + 2, strlen(msg[i]));
		if (!_pad(padding.left))
			return 0;
		if (!set_color_fg(colors.fg))
			return 0;
		if (fputs(msg[i], stdout) == EOF)
			return 0;
		if (!_pad(padding.right))
			return 0;
		if (!_move_to(root_x, root_y++))
			return 0;
	}
	if (!fputs("\x1b[m", stdout))
		return 0;
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

static inline u8	_move_to(const u32 x, const u32 y) {
	return (fprintf(stdout, "\x1b[%u;%uH", y, x) != -1) ? 1 : 0;
}

static inline u8	_putc_at(const u32 x, const u32 y, const i16 hl[2], const i32 cp) {
	if (!_move_to(x, y))
		return 0;
	if (hl[0] != -1 && set_color_fg(hl[0]) == -1)
		return 0;
	if (hl[1] != -1 && set_color_bg(hl[1]) == -1)
		return 0;
	if (fputc_utf8(cp, stdout) == EOF)
		return 0;
	return fputs("\x1b[m", stdout) != EOF;
}

static inline u8	_puts_at(const u32 x, const u32 y, const i16 hl[2], const char *s) {
	if (!_move_to(x, y))
		return 0;
	if (hl[0] != -1 && set_color_fg(hl[0]) == -1)
		return 0;
	if (hl[1] != -1 && set_color_bg(hl[1]) == -1)
		return 0;
	if (fputs(s, stdout) == EOF)
		return 0;
	return fputs("\x1b[m", stdout) != EOF;
}

static inline u8	_printf_at(const u32 x, const u32 y, const i16 hl[2], const char *fmt, ...) {
	va_list	args;
	ssize_t	rv;

	if (!_move_to(x, y))
		return 0;
	if (hl[0] != -1 && set_color_fg(hl[0]) == -1)
		return 0;
	if (hl[1] != -1 && set_color_bg(hl[1]) == -1)
		return 0;
	va_start(args, fmt);
	rv = vfprintf(stdout, fmt, args);
	va_end(args);
	if (rv == -1)
		return 0;
	return fputs("\x1b[m", stdout) != EOF;
}

static inline u8	_draw_box(const u32 root_x, const u32 root_y, const u32 width, const u32 height) {
	u32	cp;
	u32	i;

	for (i = 0; i < width; i++) {
		if (i == 0)
			cp = _BOX_CORNER_TL;
		else if (i == width - 1)
			cp = _BOX_CORNER_TR;
		else
			cp = _BOX_SIDE_HORIZONTAL;
		if (!_putc_at(root_x + i, root_y, (i16[2]){colors.edge, -1}, cp))
			return 0;
	}
	for (i = 1; i < height - 1; i++) {
		if (!_putc_at(root_x, root_y + i, (i16[2]){colors.edge, -1}, _BOX_SIDE_VERTICAL))
			return 0;
		if (!_putc_at(root_x + width - 1, root_y + i, (i16[2]){colors.edge, -1}, _BOX_SIDE_VERTICAL))
			return 0;
	}
	for (i = 0; i < width; i++) {
		if (i == 0)
			cp = _BOX_CORNER_BL;
		else if (i == width - 1)
			cp = _BOX_CORNER_BR;
		else
			cp = _BOX_SIDE_HORIZONTAL;
		if (!_putc_at(root_x + i, root_y + height - 1, (i16[2]){colors.edge, -1}, cp))
			return 0;
	}
	return 1;
}

static inline u8	_draw_paddle(const f32 paddle_pos, const u32 root_x, const u32 root_y, const u32 offset) {
	static const f32	paddle_height = 1.5f;
	size_t				dots;
	size_t				i;
	struct {
		f32	top_y;
		u32	top_char;
		u32	bot_char;
	}					paddle;
	f32					remainder;
	f32					y;

	paddle.top_y = paddle_pos - paddle_height;
	remainder = fmodf(paddle.top_y, 0.25);
	if (remainder >= 0.25 / 2)
		paddle.top_y = paddle.top_y - remainder + 0.25;
	else
		paddle.top_y = paddle.top_y - remainder;
	y = paddle.top_y - floorf(paddle.top_y);
	if (y < 0.25f)
		paddle.top_char = _PADDLE_FULL;
	else if (y < 0.5f)
		paddle.top_char = _PADDLE_3Q_UP;
	else if (y < 0.75f)
		paddle.top_char = _PADDLE_H_UP;
	else
		paddle.top_char = _PADDLE_1Q_UP;
	switch (paddle.top_char) {
		case _PADDLE_FULL:
			paddle.bot_char = _PADDLE_FULL;
			break ;
		case _PADDLE_3Q_UP:
			paddle.bot_char = _PADDLE_1Q_DOWN;
			break ;
		case _PADDLE_H_UP:
			paddle.bot_char = _PADDLE_H_DOWN;
			break ;
		case _PADDLE_1Q_UP:
			paddle.bot_char = _PADDLE_3Q_DOWN;
	}
	paddle.top_y = floorf(paddle.top_y);
	for (i = dots = 0; dots < 12; i++) {
		if (i == 0) {
			if (!_putc_at(root_x + offset, root_y + (u32)paddle.top_y, (i16[2]){colors.paddle, -1}, paddle.top_char))
				return 0;
			switch (paddle.top_char) {
				case _PADDLE_FULL:
					dots++;
					[[fallthrough]];
				case _PADDLE_3Q_UP:
					dots++;
					[[fallthrough]];
				case _PADDLE_H_UP:
					dots++;
					[[fallthrough]];
				case _PADDLE_1Q_UP:
					dots++;
			}
		} else {
			if (dots > 8) switch (dots) {
				case 9:
					return _putc_at(root_x + offset, root_y + (i32)paddle.top_y + i, (i16[2]){colors.paddle, -1}, _PADDLE_3Q_DOWN);
				case 10:
					return _putc_at(root_x + offset, root_y + (i32)paddle.top_y + i, (i16[2]){colors.paddle, -1}, _PADDLE_H_DOWN);
				case 11:
					return _putc_at(root_x + offset, root_y + (i32)paddle.top_y + i, (i16[2]){colors.paddle, -1}, _PADDLE_1Q_DOWN);
			} else if (!_putc_at(root_x + offset, root_y + (i32)paddle.top_y + i, (i16[2]){colors.paddle, -1}, _PADDLE_FULL))
				return 0;
			dots += 4;
		}
	}
	return 1;
}

static inline u8	_draw_ball(const f32 ball_pos[2], const u32 root_x, const u32 root_y) {
	static const f32	ball_radius = 0.5f;
	u32					ball_root_x;
	u32					ball_root_y;

	ball_root_x = (u32)floorf(ball_pos[0] - ball_radius);
	ball_root_y = (u32)floorf(ball_pos[1] - ball_radius);
	_putc_at(root_x + ball_root_x + 1, root_y + ball_root_y, (i16[2]){colors.ball, -1}, _BALL_TL);
	_putc_at(root_x + ball_root_x + 2, root_y + ball_root_y, (i16[2]){colors.ball, -1}, _BALL_TR);
	_putc_at(root_x + ball_root_x + 1, root_y + ball_root_y + 1, (i16[2]){colors.ball, -1}, _BALL_BL);
	_putc_at(root_x + ball_root_x + 2, root_y + ball_root_y + 1, (i16[2]){colors.ball, -1}, _BALL_BR);
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
	fputs("\x1b[2J", stdout);
	_draw_box(root_x - 1, root_y - 1, menu_width + 2, menu_height + 2);
	if (!_move_to(root_x, root_y++))
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
		if (!_move_to(root_x, root_y++))
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
