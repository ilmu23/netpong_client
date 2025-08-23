// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<game.c>>

#include <sys/socket.h>
#include <kbinput/kbinput.h>

#include "data.h"

typedef const kbinput_key	*(*game_fn)(const kbinput_key *);

typedef enum __direction {
	UP = 0,
	DOWN = 1,
	STOP = 2
}	direction;

extern kbinput_listener_id	game_binds;
extern u8					kb_protocol;

struct {
	i32	state;
	i32	p1;
	i32	p2;
}	sockets;

u8	srv_version;
u8	running;

static inline const kbinput_key	*_p1_move_paddle(const kbinput_key *event);
static inline const kbinput_key	*_p2_move_paddle(const kbinput_key *event);
static inline const kbinput_key	*_p1_pause(const kbinput_key *event);
static inline const kbinput_key	*_p2_pause(const kbinput_key *event);
static inline const kbinput_key	*_p1_quit(const kbinput_key *event);
static inline const kbinput_key	*_p2_quit(const kbinput_key *event);

static inline u8	_move_paddle(const i32 socket, const direction direction);
static inline u8	_pause(const i32 socket);
static inline u8	_quit(const i32 socket);

static inline u8	_send_msg(const i32 socket, const message *msg);
//static inline u8	_recv_msg(const i32 socket, message *msg);

u8	setup_game_binds() {
	u8	rv;

	rv = 1;
	switch (kb_protocol) {
		case KB_INPUT_PROTOCOL_KITTY:
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key('w', KB_MOD_IGN_LCK, KB_EVENT_RELEASE, _p1_move_paddle));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key('s', KB_MOD_IGN_LCK, KB_EVENT_RELEASE, _p1_move_paddle));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key(KB_KEY_UP, KB_MOD_IGN_LCK, KB_EVENT_RELEASE, _p2_move_paddle));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key(KB_KEY_DOWN, KB_MOD_IGN_LCK, KB_EVENT_RELEASE, _p2_move_paddle));
			[[fallthrough]];
		case KB_INPUT_PROTOCOL_LEGACY:
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key('w', KB_MOD_IGN_LCK, KB_EVENT_PRESS, _p1_move_paddle));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key('s', KB_MOD_IGN_LCK, KB_EVENT_PRESS, _p1_move_paddle));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key(KB_KEY_UP, KB_MOD_IGN_LCK, KB_EVENT_PRESS, _p2_move_paddle));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key(KB_KEY_DOWN, KB_MOD_IGN_LCK, KB_EVENT_PRESS, _p2_move_paddle));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key('p', KB_MOD_IGN_LCK, KB_EVENT_PRESS, _p1_pause));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key('q', KB_MOD_IGN_LCK, KB_EVENT_PRESS, _p1_quit));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key('p', KB_MOD_IGN_LCK | KB_MOD_SHIFT, KB_EVENT_PRESS, _p2_pause));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key('q', KB_MOD_IGN_LCK | KB_MOD_SHIFT, KB_EVENT_PRESS, _p2_quit));
	}
	return rv;
}

#include <stdio.h>

u8	play(void) {
	fputs("Playing!\n", stderr);
	return 1;
}

static inline const kbinput_key	*_p1_move_paddle(const kbinput_key *event) {
	static direction	direction = STOP;
	u8					rv;

	switch (kb_protocol) {
		case KB_INPUT_PROTOCOL_KITTY:
			if (event->event_type == KB_EVENT_RELEASE)
				rv = _move_paddle(sockets.p1, STOP);
			else
				rv = _move_paddle(sockets.p1, (event->code == 'w') ? UP : DOWN);
			break ;
		case KB_INPUT_PROTOCOL_LEGACY:
			if (event->code == KB_KEY_UP)
				direction = (direction != UP) ? UP : STOP;
			else
				direction = (direction != DOWN) ? DOWN : STOP;
			rv = _move_paddle(sockets.p1, direction);
	}
	return (rv) ? event : NULL;
}

static inline const kbinput_key	*_p2_move_paddle(const kbinput_key *event) {
	static direction	direction = STOP;
	u8					rv;

	switch (kb_protocol) {
		case KB_INPUT_PROTOCOL_KITTY:
			if (((const kbinput_key *)event)->event_type == KB_EVENT_RELEASE)
				rv = _move_paddle(sockets.p2, STOP);
			else
				rv = _move_paddle(sockets.p2, (event->code == 'w') ? UP : DOWN);
			break ;
		case KB_INPUT_PROTOCOL_LEGACY:
			if (event->code == KB_KEY_UP)
				direction = (direction != UP) ? UP : STOP;
			else
				direction = (direction != DOWN) ? DOWN : STOP;
			rv = _move_paddle(sockets.p2, direction);
	}
	return (rv) ? event : NULL;
}

static inline const kbinput_key	*_p1_pause(const kbinput_key *event) {
	return (_pause(sockets.p1)) ? event : NULL;
}

static inline const kbinput_key	*_p2_pause(const kbinput_key *event) {
	return (_pause(sockets.p2)) ? event : NULL;
}

static inline const kbinput_key	*_p1_quit(const kbinput_key *event) {
	return (_quit(sockets.p1)) ? event : NULL;
}

static inline const kbinput_key	*_p2_quit(const kbinput_key *event) {
	return (_quit(sockets.p2)) ? event : NULL;
}

static inline u8	_move_paddle(const i32 socket, const direction direction) {
	message	msg;

	msg = (message){
		.version = srv_version,
		.type = MESSAGE_CLIENT_MOVE_PADDLE,
		.length = sizeof(msg_clt_move_paddle),
	};
	*(msg_clt_move_paddle *)msg.body = (msg_clt_move_paddle){
		.direction = direction
	};
	return _send_msg(socket, &msg);
}

static inline u8	_pause(const i32 socket) {
	message	msg;

	msg = (message){
		.version = srv_version,
		.type = MESSAGE_CLIENT_PAUSE,
		.length = 0
	};
	return _send_msg(socket, &msg);
}

static inline u8	_quit(const i32 socket) {
	message	msg;

	msg = (message){
		.version = srv_version,
		.type = MESSAGE_CLIENT_QUIT,
		.length = 0
	};
	return _send_msg(socket, &msg);
}

static inline u8	_send_msg(const i32 socket, const message *msg) {
	return (send(socket, msg, MESSAGE_HEADER_SIZE + msg->length, 0) == MESSAGE_HEADER_SIZE + msg->length) ? 1 : 0;
}
