// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<menu.c>>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <kbinput/kbinput.h>

#include "menu.h"
#include "game.h"
#include "display.h"

#define CSI	"\x1b["

#define _COLOR_CODE_COUNT	256

#define _SMCUP	CSI "?1049h"
#define _RMCUP	CSI "?1049l"

#define _TERM_ALT_SCREEN	_SMCUP
#define _TERM_MAIN_SCREEN	_RMCUP

typedef const kbinput_key	*(*menu_fn)(const kbinput_key *);

extern struct {
	const char	*addr;
	const char	*port;
	struct {
		i32	state;
		i32	p1;
		i32	p2;
	}	sockets;
	u8	srv_version;
	u8	running;
}	server_info;

extern struct {
	u8	fg;
	struct selection {
		u8	fg;
		u8	bg;
	}	selection;
	u8	paddle;
	u8	ball;
	u8	edge;
}	colors;

kbinput_listener_id	menu_binds;
kbinput_listener_id	game_binds;
u8					kb_protocol;

struct {
	menu	*main;
	menu	*options;
	struct {
		menu	*fg;
		struct {
			menu	*fg;
			menu	*bg;
		}		selection;
		menu	*paddle;
		menu	*ball;
		menu	*edge;
	}		colors;
	menu	*current;
}	menus;

static inline const kbinput_key	*_navigate(const kbinput_key *event);
static inline const kbinput_key	*_select(const kbinput_key *event);
static inline const kbinput_key	*_back(const kbinput_key *event);
static inline const kbinput_key	*_quit(const kbinput_key *event);

static inline void	_set_fg_color(const uintptr_t val);
static inline void	_set_selection_fg_color(const uintptr_t val);
static inline void	_set_selection_bg_color(const uintptr_t val);
static inline void	_set_paddle_color(const uintptr_t val);
static inline void	_set_ball_color(const uintptr_t val);
static inline void	_set_edge_color(const uintptr_t val);

static inline u8	_init(const char *server_addr, const char *server_port);
static inline u8	_setup_menu_binds(void);

static inline void	_free_menu(menu *menu);
static inline u8	_setup_menus(void);
static inline u8	_setup_colormenu(menu *_menu, const menu *prev, opt_setter setter);

static inline menu_item	*_new_item(const char *title, const menu *prev, const menu *next, const menu_action action, opt_setter setter, const uintptr_t opt_val);
static inline u8		_add_right(menu_item *ref, menu_item *new);
static inline u8		_add_below(menu_item *ref, menu_item *new);

static const char	*color_codes[_COLOR_CODE_COUNT] = {
	"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15",
	"16", "17", "18", "19", "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "30",
	"31", "32", "33", "34", "35", "36", "37", "38", "39", "40", "41", "42", "43", "44", "45",
	"46", "47", "48", "49", "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "60",
	"61", "62", "63", "64", "65", "66", "67", "68", "69", "70", "71", "72", "73", "74", "75",
	"76", "77", "78", "79", "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "90",
	"91", "92", "93", "94", "95", "96", "97", "98", "99", "100", "101", "102", "103", "104", "105",
	"106", "107", "108", "109", "110", "111", "112", "113", "114", "115", "116", "117", "118", "119",
	"120", "121", "122", "123", "124", "125", "126", "127", "128", "129", "130", "131", "132", "133",
	"134", "135", "136", "137", "138", "139", "140", "141", "142", "143", "144", "145", "146", "147",
	"148", "149", "150", "151", "152", "153", "154", "155", "156", "157", "158", "159", "160", "161",
	"162", "163", "164", "165", "166", "167", "168", "169", "170", "171", "172", "173", "174", "175",
	"176", "177", "178", "179", "180", "181", "182", "183", "184", "185", "186", "187", "188", "189",
	"190", "191", "192", "193", "194", "195", "196", "197", "198", "199", "200", "201", "202", "203",
	"204", "205", "206", "207", "208", "209", "210", "211", "212", "213", "214", "215", "216", "217",
	"218", "219", "220", "221", "222", "223", "224", "225", "226", "227", "228", "229", "230", "231",
	"232", "233", "234", "235", "236", "237", "238", "239", "240", "241", "242", "243", "244", "245",
	"246", "247", "248", "249", "250", "251", "252", "253", "254", "255"
};

u8	main_menu(const char *server_addr, const char *server_port) {
	const kbinput_key	*event;
	u8					rv;

	if (!_init(server_addr, server_port))
		return 0;
	rv = 1;
	kbinput_set_cursor_mode(OFF);
	while (rv) {
		display_menu(menus.current);
		event = kbinput_listen(menu_binds);
		if (!event) {
			if (errno == EINTR) {
				errno = 0;
				continue ;
			}
			rv = 0;
			break ;
		} else if ((menu_fn)event->fn == _quit)
			break ;
		if (!((menu_fn)event->fn)(event)) {
			if ((menu_fn)event->fn == _select)
				break ;
			rv = 0;
		}
	}
	cleanup();
	return rv;
}

void	cleanup(void) {
	kbinput_cleanup();
	write(1, _TERM_MAIN_SCREEN, sizeof(_TERM_MAIN_SCREEN));
	_free_menu(menus.colors.selection.bg);
	_free_menu(menus.colors.selection.fg);
	_free_menu(menus.colors.fg);
	_free_menu(menus.main);
}

static inline const kbinput_key	*_navigate(const kbinput_key *event) {
	menu_item	*next;

	switch (event->code) {
		case 'h':
		case 'a':
		case KB_KEY_LEFT:
			next = menus.current->current->neighbors.left;
			break ;
		case 'j':
		case 's':
		case KB_KEY_DOWN:
			next = menus.current->current->neighbors.down;
			break ;
		case 'k':
		case 'w':
		case KB_KEY_UP:
			next = menus.current->current->neighbors.up;
			break ;
		case 'l':
		case 'd':
		case KB_KEY_RIGHT:
			next = menus.current->current->neighbors.right;
	}
	if (next) {
		menus.current->current->selected = 0;
		menus.current->current = next;
		next->selected = 1;
	}
	return event;
}

static inline const kbinput_key	*_select(const kbinput_key *event) {
	switch (menus.current->current->action) {
		case PLAY:
			return (play()) ? event : NULL;
		case LOGIN:
			break ;
		case ENTER_MENU:
			menus.current = (menu *)menus.current->current->next;
			if (menus.current->current != menus.current->root) {
				menus.current->current->selected = 0;
				menus.current->current = menus.current->root;
				menus.current->current->selected = 1;
			}
			break ;
		case SELECT_OPTION:
			menus.current->current->option.set(menus.current->current->option.val);
			[[fallthrough]];
		case BACK:
			_back(event);
			break ;
		case EXIT:
			return NULL;
	}
	return event;
}

static inline const kbinput_key	*_back(const kbinput_key *event) {
	if (menus.current->current->prev)
		menus.current = (menu *)menus.current->current->prev;
	return event;
}

static inline const kbinput_key	*_quit([[gnu::unused]] const kbinput_key *event) {
	return NULL;
}

static inline void	_set_fg_color(const uintptr_t val) {
	colors.fg = val;
}

static inline void	_set_selection_fg_color(const uintptr_t val) {
	colors.selection.fg = val;
}

static inline void	_set_selection_bg_color(const uintptr_t val) {
	colors.selection.bg = val;
}

static inline void	_set_paddle_color(const uintptr_t val) {
	colors.paddle = val;
}

static inline void	_set_ball_color(const uintptr_t val) {
	colors.ball = val;
}

static inline void	_set_edge_color(const uintptr_t val) {
	colors.edge = val;
}

static inline u8	_init(const char *server_addr, const char *server_port) {
	if (write(1, _TERM_ALT_SCREEN, sizeof(_TERM_ALT_SCREEN)) != sizeof(_TERM_ALT_SCREEN))
		return 0;
	kbinput_init();
	kb_protocol = kbinput_get_input_protocol();
	if (kb_protocol == KB_INPUT_PROTOCOL_ERROR)
		return 0;
	menu_binds = kbinput_new_listener();
	game_binds = kbinput_new_listener();
	if (menu_binds == -1 || game_binds == -1)
		return 0;
	if (!_setup_menu_binds() || !setup_game_binds())
		return 0;
	setvbuf(stdout, NULL, _IOFBF, 4096);
	if (!_setup_menus())
		return 0;
	server_info.addr = server_addr;
	server_info.port = server_port;
	return init_display();
}

static inline u8	_setup_menu_binds(void) {
	u8	rv;

	rv = 1;
	switch (kb_protocol) {
		case KB_INPUT_PROTOCOL_KITTY:
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key('h', KB_MOD_IGN_LCK, KB_EVENT_REPEAT, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key('j', KB_MOD_IGN_LCK, KB_EVENT_REPEAT, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key('k', KB_MOD_IGN_LCK, KB_EVENT_REPEAT, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key('l', KB_MOD_IGN_LCK, KB_EVENT_REPEAT, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key('w', KB_MOD_IGN_LCK, KB_EVENT_REPEAT, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key('a', KB_MOD_IGN_LCK, KB_EVENT_REPEAT, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key('s', KB_MOD_IGN_LCK, KB_EVENT_REPEAT, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key('d', KB_MOD_IGN_LCK, KB_EVENT_REPEAT, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key(KB_KEY_UP, KB_MOD_IGN_LCK, KB_EVENT_REPEAT, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key(KB_KEY_DOWN, KB_MOD_IGN_LCK, KB_EVENT_REPEAT, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key(KB_KEY_LEFT, KB_MOD_IGN_LCK, KB_EVENT_REPEAT, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key(KB_KEY_RIGHT, KB_MOD_IGN_LCK, KB_EVENT_REPEAT, _navigate));
			[[fallthrough]];
		case KB_INPUT_PROTOCOL_LEGACY:
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key('h', KB_MOD_IGN_LCK, KB_EVENT_PRESS, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key('j', KB_MOD_IGN_LCK, KB_EVENT_PRESS, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key('k', KB_MOD_IGN_LCK, KB_EVENT_PRESS, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key('l', KB_MOD_IGN_LCK, KB_EVENT_PRESS, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key('w', KB_MOD_IGN_LCK, KB_EVENT_PRESS, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key('a', KB_MOD_IGN_LCK, KB_EVENT_PRESS, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key('s', KB_MOD_IGN_LCK, KB_EVENT_PRESS, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key('d', KB_MOD_IGN_LCK, KB_EVENT_PRESS, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key(KB_KEY_UP, KB_MOD_IGN_LCK, KB_EVENT_PRESS, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key(KB_KEY_DOWN, KB_MOD_IGN_LCK, KB_EVENT_PRESS, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key(KB_KEY_LEFT, KB_MOD_IGN_LCK, KB_EVENT_PRESS, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key(KB_KEY_RIGHT, KB_MOD_IGN_LCK, KB_EVENT_PRESS, _navigate));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key(KB_KEY_ENTER, KB_MOD_IGN_LCK, KB_EVENT_PRESS, _select));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key(KB_KEY_BACKSPACE, KB_MOD_IGN_LCK, KB_EVENT_PRESS, _back));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key(KB_KEY_ESCAPE, KB_MOD_IGN_LCK, KB_EVENT_PRESS, _quit));
			rv ^= ~kbinput_add_listener(menu_binds, kbinput_key('q', KB_MOD_IGN_LCK, KB_EVENT_PRESS, _quit));
	}
	return rv;
}

static inline void	_free_menu(menu *menu) {
	menu_item	*right;
	menu_item	*tmp;

	while (menu->root->neighbors.up)
		menu->root = menu->root->neighbors.up;
	while (menu->root) {
		right = menu->root->neighbors.right;
		if (right) {
			while (right->neighbors.right)
				right = right->neighbors.right;
			for (tmp = right->neighbors.left; right != menu->root; right = tmp, tmp = tmp->neighbors.left)
				free(right);
		}
		tmp = menu->root->neighbors.down;
		free(menu->root);
		menu->root = tmp;
	}
	free(menu);
}

static inline u8	_setup_menus(void) {
	menu_item	*tmp;

	menus.main = malloc(sizeof(*menus.main));
	menus.options = malloc(sizeof(*menus.options));
	menus.colors.fg = malloc(sizeof(*menus.colors.fg));
	menus.colors.selection.fg = malloc(sizeof(*menus.colors.selection.fg));
	menus.colors.selection.bg = malloc(sizeof(*menus.colors.selection.bg));
	menus.colors.paddle = malloc(sizeof(*menus.colors.paddle));
	menus.colors.ball = malloc(sizeof(*menus.colors.ball));
	menus.colors.edge = malloc(sizeof(*menus.colors.edge));
	if (!menus.main || !menus.options || !menus.colors.fg || !menus.colors.selection.fg ||
		!menus.colors.selection.bg || !menus.colors.paddle || !menus.colors.ball || !menus.colors.edge)
		return 0;
	*menus.main = (menu){
		.root = _new_item("Start Game", NULL, NULL, PLAY, NULL, 0),
		.width = 1,
		.height = 1,
		.longest_title = strlen("Start Game")
	};
	*menus.options = (menu){
		.root = _new_item("Set Foreground Color", menus.main, menus.colors.fg, ENTER_MENU, NULL, 0),
		.width = 1,
		.height = 1,
		.longest_title = strlen("Set Foreground Color")
	};
	*menus.colors.fg = (menu){
		.root = _new_item(color_codes[0], menus.options, NULL, SELECT_OPTION, _set_fg_color, 0),
		.width = 8,
		.height = 1,
		.longest_title = 3
	};
	*menus.colors.selection.fg = (menu){
		.root = _new_item(color_codes[0], menus.options, NULL, SELECT_OPTION, _set_selection_fg_color, 0),
		.width = 8,
		.height = 1,
		.longest_title = 3
	};
	*menus.colors.selection.bg = (menu){
		.root = _new_item(color_codes[0], menus.options, NULL, SELECT_OPTION, _set_selection_bg_color, 0),
		.width = 8,
		.height = 1,
		.longest_title = 3
	};
	*menus.colors.paddle = (menu){
		.root = _new_item(color_codes[0], menus.options, NULL, SELECT_OPTION, _set_paddle_color, 0),
		.width = 8,
		.height = 1,
		.longest_title = 3
	};
	*menus.colors.ball = (menu){
		.root = _new_item(color_codes[0], menus.options, NULL, SELECT_OPTION, _set_ball_color, 0),
		.width = 8,
		.height = 1,
		.longest_title = 3
	};
	*menus.colors.edge = (menu){
		.root = _new_item(color_codes[0], menus.options, NULL, SELECT_OPTION, _set_edge_color, 0),
		.width = 8,
		.height = 1,
		.longest_title = 3
	};
	if (!menus.main->root || !menus.options->root || !menus.colors.fg->root ||
		!menus.colors.selection.fg->root || !menus.colors.selection.bg || !menus.colors.paddle->root ||
		!menus.colors.ball->root || !menus.colors.edge)
		return 0;
	menus.main->current = menus.main->root;
	menus.options->current = menus.options->root;
	menus.colors.fg->current = menus.colors.fg->root;
	menus.colors.selection.fg->current = menus.colors.selection.fg->root;
	menus.colors.selection.bg->current = menus.colors.selection.bg->root;
	menus.colors.paddle->current = menus.colors.paddle->root;
	menus.colors.ball->current = menus.colors.ball->root;
	menus.colors.edge->current = menus.colors.edge->root;
	tmp = menus.main->root;
	if (!_add_below(tmp, _new_item("Options", NULL, menus.options, ENTER_MENU, NULL, 0)))
		return 0;
	menus.main->height++;
	tmp = tmp->neighbors.down;
	if (strlen(tmp->title) > menus.main->longest_title)
		menus.main->longest_title = strlen(tmp->title);
	if (!_add_below(tmp, _new_item("Exit", NULL, NULL, EXIT, NULL, 0)))
		return 0;
	menus.main->height++;
	tmp = tmp->neighbors.down;
	if (strlen(tmp->title) > menus.main->longest_title)
		menus.main->longest_title = strlen(tmp->title);
	tmp = menus.options->root;
	if (!_add_below(tmp, _new_item("Set Selection Foreground Color", menus.options, menus.colors.selection.fg, ENTER_MENU, NULL, 0)))
		return 0;
	menus.options->height++;
	tmp = tmp->neighbors.down;
	if (strlen(tmp->title) > menus.options->longest_title)
		menus.options->longest_title = strlen(tmp->title);
	if (!_add_below(tmp, _new_item("Set Selection Background Color", menus.options, menus.colors.selection.bg, ENTER_MENU, NULL, 0)))
		return 0;
	menus.options->height++;
	tmp = tmp->neighbors.down;
	if (strlen(tmp->title) > menus.options->longest_title)
		menus.options->longest_title = strlen(tmp->title);
	if (!_add_below(tmp, _new_item("Set Paddle Color", menus.options, menus.colors.paddle, ENTER_MENU, NULL, 0)))
		return 0;
	menus.options->height++;
	tmp = tmp->neighbors.down;
	if (strlen(tmp->title) > menus.options->longest_title)
		menus.options->longest_title = strlen(tmp->title);
	if (!_add_below(tmp, _new_item("Set Ball Color", menus.options, menus.colors.ball, ENTER_MENU, NULL, 0)))
		return 0;
	menus.options->height++;
	tmp = tmp->neighbors.down;
	if (strlen(tmp->title) > menus.options->longest_title)
		menus.options->longest_title = strlen(tmp->title);
	if (!_add_below(tmp, _new_item("Set Edge Color", menus.options, menus.colors.edge, ENTER_MENU, NULL, 0)))
		return 0;
	menus.options->height++;
	tmp = tmp->neighbors.down;
	if (strlen(tmp->title) > menus.options->longest_title)
		menus.options->longest_title = strlen(tmp->title);
	if (!_add_below(tmp, _new_item("Back", menus.main, NULL, BACK, NULL, 0)))
		return 0;
	menus.options->height++;
	tmp = tmp->neighbors.down;
	if (strlen(tmp->title) > menus.options->longest_title)
		menus.options->longest_title = strlen(tmp->title);
	menus.colors.selection.bg->root->selected = 1;
	menus.colors.selection.fg->root->selected = 1;
	menus.colors.paddle->root->selected = 1;
	menus.colors.ball->root->selected = 1;
	menus.colors.edge->root->selected = 1;
	menus.colors.fg->root->selected = 1;
	menus.options->root->selected = 1;
	menus.main->root->selected = 1;
	menus.current = menus.main;
	return _setup_colormenu(menus.colors.fg, menus.options, _set_fg_color)
		&& _setup_colormenu(menus.colors.selection.fg, menus.options, _set_selection_fg_color)
		&& _setup_colormenu(menus.colors.selection.bg, menus.options, _set_selection_bg_color)
		&& _setup_colormenu(menus.colors.paddle, menus.options, _set_paddle_color)
		&& _setup_colormenu(menus.colors.ball, menus.options, _set_ball_color)
		&& _setup_colormenu(menus.colors.edge, menus.options, _set_edge_color);
}

static inline u8	_setup_colormenu(menu *_menu, const menu *prev, opt_setter setter) {
	menu_item	*down;
	menu_item	*tmp;
	size_t		i;

	for (i = 1, tmp = down = _menu->root; i < _COLOR_CODE_COUNT; i++) {
		if (i && i % 8) {
			if (!_add_right(tmp, _new_item(color_codes[i], prev, NULL, SELECT_OPTION, setter, i)))
				return 0;
			tmp = tmp->neighbors.right;
		} else {
			if (!_add_below(down, _new_item(color_codes[i], prev, NULL, SELECT_OPTION, setter, i)))
				return 0;
			down = down->neighbors.down;
			_menu->height++;
			tmp = down;
		}
	}
	return 1;
}

static inline menu_item	*_new_item(const char *title, const menu *prev, const menu *next, const menu_action action, opt_setter setter, const uintptr_t opt_val) {
	menu_item	*out;

	out = malloc(sizeof(*out));
	if (out) {
		*out = (menu_item){
			.neighbors.up = NULL,
			.neighbors.down = NULL,
			.neighbors.left = NULL,
			.neighbors.right = NULL,
			.option.set = setter,
			.option.val = opt_val,
			.title = title,
			.next = next,
			.prev = prev,
			.action = action,
			.selected = 0
		};
	}
	return out;
}

static inline u8	_add_right(menu_item *ref, menu_item *new) {
	if (!new)
		return 0;
	if (ref->neighbors.right) {
		free(new);
		return 0;
	}
	new->neighbors.left = ref;
	ref->neighbors.right = new;
	if (ref->neighbors.up && ref->neighbors.up->neighbors.right) {
		new->neighbors.up = ref->neighbors.up->neighbors.right;
		new->neighbors.up->neighbors.down = new;
	}
	if (ref->neighbors.down && ref->neighbors.down->neighbors.right) {
		new->neighbors.down = ref->neighbors.down->neighbors.right;
		new->neighbors.down->neighbors.up = new;
	}
	return 1;
}

static inline u8	_add_below(menu_item *ref, menu_item *new) {
	if (!new)
		return 0;
	if (ref->neighbors.down) {
		free(new);
		return 0;
	}
	new->neighbors.up = ref;
	ref->neighbors.down = new;
	if (ref->neighbors.left && ref->neighbors.left->neighbors.down) {
		new->neighbors.left = ref->neighbors.left->neighbors.down;
		new->neighbors.left->neighbors.right = new;
	}
	if (ref->neighbors.right && ref->neighbors.right->neighbors.down) {
		new->neighbors.right = ref->neighbors.right->neighbors.down;
		new->neighbors.right->neighbors.left = new;
	}
	return 1;
}
