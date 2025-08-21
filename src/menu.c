// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<menu.c>>

#include <unistd.h>
#include <kbinput/kbinput.h>

#include "game.h"

#define CSI	"\x1b["

#define _SMCUP	CSI "?1049h"
#define _RMCUP	CSI "?1049l"

#define _TERM_ALT_SCREEN	_SMCUP
#define _TERM_MAIN_SCREEN	_RMCUP

kbinput_listener_id	menu_binds;
kbinput_listener_id	game_binds;
u8					kb_protocol;

static inline void	*_navigate(void *event);
static inline void	*_select(void *event);
static inline void	*_back(void *event);
static inline void	*_quit(void *event);

static inline u8	_init(void);
static inline u8	_cleanup(const u8 rv);
static inline u8	_setup_menu_binds(void);

u8	start_menu(void) {
	const kbinput_key	*event;
	u8					rv;

	if (!_init())
		return 0;
	rv = 1;
	while (rv) {
		event = kbinput_listen(menu_binds);
		if (!event)
			rv = 0;
		else if (event->fn == _quit)
			break ;
		if (!event->fn((kbinput_key *)event))
			rv = 0;
	}
	return _cleanup(rv);
}

static inline u8	_init(void) {
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
	return 1;
}

static inline u8	_cleanup(const u8 rv) {
	kbinput_cleanup();
	write(1, _TERM_MAIN_SCREEN, sizeof(_TERM_MAIN_SCREEN));
	return rv;
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
	}
	return rv;
}

#include <stdio.h>

static inline void	*_navigate(void *event) {
	fputs("Navigating!\n", stderr);
	return event;
}

static inline void	*_select(void *event) {
	fputs("Selecting!\n", stderr);
	return (play()) ? event : NULL;
}

static inline void	*_back(void *event) {
	fputs("Going back!\n", stderr);
	return event;
}

static inline void	*_quit([[gnu::unused]] void *event) {
	return NULL;
}
