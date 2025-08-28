// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<game.c>>

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <kbinput/kbinput.h>

#include "data.h"
#include "game.h"
#include "utils.h"
#include "display.h"

#define atomic	_Atomic

#define _SIGS	kb_io_listener.sigs.set

#define _GAME_FIELD_X	40.0f
#define _GAME_FIELD_Y	20.0f

#define _MSG_GAME_OVER			"Game over"
#define _MSG_GAME_OVER_P1_WON	"Player 1 wins"
#define _MSG_GAME_OVER_P2_WON	"Player 2 wins"
#define _MSG_SERVER_CLOSED		"Server closed"
#define _MSG_P1_QUIT			"Player 1 quit"
#define _MSG_P2_QUIT			"Player 2 quit"

#define add_sig(set, sig)	((sigaddset(&set, sig) != -1) ? 1 : 0)

typedef const kbinput_key	*(*game_fn)(const kbinput_key *);

typedef enum __direction {
	UP = 0x1,
	DOWN = 0x2,
	STOP = 0x0
}	direction;

typedef enum {
	PLAYER1_WON,
	PLAYER2_WON,
	PLAYER1_QUIT,
	PLAYER2_QUIT,
	SERVER_CLOSED
}	message_type;

extern kbinput_listener_id	game_binds;
extern u8					kb_protocol;

static const struct addrinfo	hints = {
	.ai_family = AF_INET,
	.ai_socktype = SOCK_STREAM,
	.ai_flags = 0,
	.ai_protocol = 0,
	.ai_addrlen = 0,
	.ai_addr = NULL,
	.ai_canonname = NULL,
	.ai_next = NULL
};

static struct {
	pthread_mutex_t	start;
	pthread_t		tid;
	struct {
		sigset_t	set;
		u8			init;
	}				sigs;
}	kb_io_listener = {
	.start = PTHREAD_MUTEX_INITIALIZER
};

static struct {
	pthread_mutex_t	lock;
	game			state;
}	_game = {
	.lock = PTHREAD_MUTEX_INITIALIZER
};

static atomic u8	display_status;

struct {
	const char	*addr;
	const char	*port;
	struct {
		i32	state;
		i32	p1;
		i32	p2;
	}	sockets;
	u8	version;
	u8	running;
	u8	direct;
}	server_info;

static void	*_kb_io_listener(void *);

static inline const kbinput_key	*_p1_move_paddle(const kbinput_key *event);
static inline const kbinput_key	*_p2_move_paddle(const kbinput_key *event);
static inline const kbinput_key	*_p1_toggle_pause(const kbinput_key *event);
static inline const kbinput_key	*_p2_toggle_pause(const kbinput_key *event);
static inline const kbinput_key	*_p1_quit(const kbinput_key *event);
static inline const kbinput_key	*_p2_quit(const kbinput_key *event);

static inline u8	_move_paddle(const i32 socket, const direction direction);
static inline u8	_toggle_pause(const i32 socket);
static inline u8	_start(const i32 socket);
static inline u8	_quit(const i32 socket);

static inline u8	_print_msg(const message_type msg, const u32 wait);

static inline void	_close(i32 fd);
static inline u8	_init_connection(void);
static inline u8	_connect(const char *addr, const char *port, i32 *sfd);
static inline u8	_send(const i32 socket, const void *buf, const size_t n);
static inline u8	_recv(const i32 socket, void *buf, const size_t n);
static inline u8	_send_msg(const i32 socket, const message *msg);
static inline u8	_recv_msg(const i32 socket, message *msg);

u8	setup_game_binds(void) {
	u8	rv;

	rv = 1;
	switch (kb_protocol) {
		case KB_INPUT_PROTOCOL_KITTY:
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key('w', KB_MOD_IGN_LCK, KB_EVENT_RELEASE, _p1_move_paddle));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key('s', KB_MOD_IGN_LCK, KB_EVENT_RELEASE, _p1_move_paddle));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key(KB_KEY_UP, KB_MOD_IGN_LCK, KB_EVENT_RELEASE, _p2_move_paddle));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key(KB_KEY_DOWN, KB_MOD_IGN_LCK, KB_EVENT_RELEASE, _p2_move_paddle));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key('w', KB_MOD_IGN_LCK, KB_EVENT_PRESS, _p1_move_paddle));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key('s', KB_MOD_IGN_LCK, KB_EVENT_PRESS, _p1_move_paddle));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key(KB_KEY_UP, KB_MOD_IGN_LCK, KB_EVENT_PRESS, _p2_move_paddle));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key(KB_KEY_DOWN, KB_MOD_IGN_LCK, KB_EVENT_PRESS, _p2_move_paddle));
			break ;
		case KB_INPUT_PROTOCOL_LEGACY:
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key('w', KB_MOD_IGN_LCK, KB_EVENT_PRESS, _p1_move_paddle));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key('s', KB_MOD_IGN_LCK, KB_EVENT_PRESS, _p1_move_paddle));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key(KB_KEY_LEGACY_UP, KB_MOD_IGN_LCK, KB_EVENT_PRESS, _p1_move_paddle));
			rv ^= ~kbinput_add_listener(game_binds, kbinput_key(KB_KEY_LEGACY_DOWN, KB_MOD_IGN_LCK, KB_EVENT_PRESS, _p1_move_paddle));
	}
	rv ^= ~kbinput_add_listener(game_binds, kbinput_key('p', KB_MOD_IGN_LCK, KB_EVENT_PRESS, _p1_toggle_pause));
	rv ^= ~kbinput_add_listener(game_binds, kbinput_key('q', KB_MOD_IGN_LCK, KB_EVENT_PRESS, _p1_quit));
	rv ^= ~kbinput_add_listener(game_binds, kbinput_key('p', KB_MOD_IGN_LCK | KB_MOD_SHIFT, KB_EVENT_PRESS, _p2_toggle_pause));
	rv ^= ~kbinput_add_listener(game_binds, kbinput_key('q', KB_MOD_IGN_LCK | KB_MOD_SHIFT, KB_EVENT_PRESS, _p2_quit));
	return rv;
}

u8	play(void) {
	message	msg;
	u8		pause_state;
	u8		rv;

	if (!_init_connection()) {
		_close(server_info.sockets.p1);
		_close(server_info.sockets.p2);
		_close(server_info.sockets.state);
		pthread_mutex_unlock(&kb_io_listener.start);
		return 0;
	}
	server_info.running = 1;
	_game.state.p1_pos = _GAME_FIELD_Y / 2;
	_game.state.p2_pos = _GAME_FIELD_Y / 2;
	_game.state.ball.x = _GAME_FIELD_X / 2;
	_game.state.ball.y = _GAME_FIELD_Y / 2;
	_game.state.p1_score = 0;
	_game.state.p2_score = 0;
	_game.state.started = 0;
	_game.state.paused = 0x3U;
	_game.state.status = 0;
	_game.state.actor = 0;
	_game.state.over = 0;
	display_status = display_game(&_game.state);
	pthread_mutex_unlock(&kb_io_listener.start);
	do {
		errno = 0;
		if (display_status == DISPLAY_GAME_WIN_TOO_SMALL) {
			pthread_mutex_lock(&_game.lock);
			display_status = display_game(&_game.state);
			pause_state = _game.state.paused;
			pthread_mutex_unlock(&_game.lock);
			switch (display_status) {
				case 0:
					rv = 0;
					break ;
				case DISPLAY_GAME_WIN_TOO_SMALL:
					if (!(pause_state & 0x1))
						_p1_toggle_pause((void *)0x1);
					if (!(pause_state & 0x2))
						_p2_toggle_pause((void *)0x1);
					continue ;
			}
			if (pause_state & 0x1)
				_p1_toggle_pause((void *)0x1);
			if (pause_state & 0x2)
				_p2_toggle_pause((void *)0x1);
		}
		rv = _recv_msg(server_info.sockets.state, &msg);
		if (!rv) {
			if (errno == EINTR) {
				rv = 1;
				continue ;
			} else if (!server_info.running)
				rv = 1;
			break ;
		}
		pthread_mutex_lock(&_game.lock);
		switch (msg.type) {
			case MESSAGE_SERVER_GAME_PAUSED:
				break ;
			case MESSAGE_SERVER_GAME_OVER:
				_game.state.paused = 1;
				_game.state.over = 1;
				switch (server_info.version) {
					case 0:
						_game.state.status = GAME_OVER_ACT_WON;
						_game.state.actor = msg.body.game_over.v0.winner_id;
						break ;
					case 1:
						_game.state.p1_score = msg.body.game_over.v1.score >> 8 & 0xFF;
						_game.state.p2_score = msg.body.game_over.v1.score & 0xFF;
						_game.state.status = msg.body.game_over.v1.finish_status;
						_game.state.actor = msg.body.game_over.v1.actor_id;
				}
				break ;
			case MESSAGE_SERVER_STATE_UPDATE:
				_game.state.paused = 0;
				_game.state.started = 1;
				_game.state.p1_pos = msg.body.state.p1_paddle;
				_game.state.p2_pos = msg.body.state.p2_paddle;
				_game.state.ball.x = msg.body.state.ball.x;
				_game.state.ball.y = msg.body.state.ball.y;
				_game.state.p1_score = msg.body.state.score >> 8 & 0xFF;
				_game.state.p2_score = msg.body.state.score & 0xFF;
		}
		display_status = display_game(&_game.state);
		pause_state = _game.state.paused;
		pthread_mutex_unlock(&_game.lock);
		switch (display_status) {
			case 0:
				rv = 0;
				break ;
			case DISPLAY_GAME_WIN_TOO_SMALL:
				if (!(pause_state & 0x1))
					_p1_toggle_pause((void *)0x1);
				if (!(pause_state & 0x2))
					_p2_toggle_pause((void *)0x1);
		}
		pthread_mutex_lock(&_game.lock);
		if (_game.state.over)
			break ;
		pthread_mutex_unlock(&_game.lock);
	} while (rv);
	pthread_mutex_unlock(&_game.lock);
	server_info.running = 0;
	pthread_cancel(kb_io_listener.tid);
	pthread_join(kb_io_listener.tid, NULL);
	write(1, "\x1b[=0u", 5);
	switch (_game.state.status) {
		case GAME_OVER_ACT_WON:
			_print_msg((_game.state.actor == 1) ? PLAYER1_WON : PLAYER2_WON, 5);
			break ;
		case GAME_OVER_ACT_QUIT:
			_print_msg((_game.state.actor == 1) ? PLAYER1_QUIT : PLAYER2_QUIT, 5);
			break ;
		case GAME_OVER_SERVER_CLOSED:
			_print_msg(SERVER_CLOSED, 5);
	}
	_close(server_info.sockets.state);
	_close(server_info.sockets.p1);
	_close(server_info.sockets.p2);
	return rv;
}

static void	*_kb_io_listener([[gnu::unused]] void *arg) {
	const kbinput_key	*event;

	pthread_sigmask(SIG_BLOCK, &_SIGS, NULL);
	pthread_mutex_lock(&kb_io_listener.start);
	pthread_mutex_unlock(&kb_io_listener.start);
	while (1) {
		event = kbinput_listen(game_binds);
		if (event && display_status != DISPLAY_GAME_WIN_TOO_SMALL)
			((game_fn)event->fn)(event);
	}
	return NULL;
}

static inline const kbinput_key	*_p1_move_paddle(const kbinput_key *event) {
	static direction	direction = STOP;

	switch (kb_protocol) {
		case KB_INPUT_PROTOCOL_KITTY:
			if (event->code == 'w') {
				if (event->event_type == KB_EVENT_PRESS)
					direction = (direction == STOP) ? UP : STOP;
				else
					direction = (direction == UP) ? STOP : DOWN;
			} else {
				if (event->event_type == KB_EVENT_PRESS)
					direction = (direction == STOP) ? DOWN : STOP;
				else
					direction = (direction == DOWN) ? STOP : UP;
			}
			break ;
		case KB_INPUT_PROTOCOL_LEGACY:
			if (event->code == 'w')
				direction = (direction != UP) ? UP : STOP;
			else
				direction = (direction != DOWN) ? DOWN : STOP;
	}
	return (_move_paddle(server_info.sockets.p1, direction)) ? event : NULL;
}

static inline const kbinput_key	*_p2_move_paddle(const kbinput_key *event) {
	static direction	direction = STOP;

	switch (kb_protocol) {
		case KB_INPUT_PROTOCOL_KITTY:
			if (event->code == KB_KEY_UP) {
				if (event->event_type == KB_EVENT_PRESS)
					direction = (direction == STOP) ? UP : STOP;
				else
					direction = (direction == UP) ? STOP : DOWN;
			} else {
				if (event->event_type == KB_EVENT_PRESS)
					direction = (direction == STOP) ? DOWN : STOP;
				else
					direction = (direction == DOWN) ? STOP : UP;
			}
			break ;
		case KB_INPUT_PROTOCOL_LEGACY:
			if (event->code == KB_KEY_LEGACY_UP)
				direction = (direction != UP) ? UP : STOP;
			else
				direction = (direction != DOWN) ? DOWN : STOP;
	}
	return (_move_paddle(server_info.sockets.p2, direction)) ? event : NULL;
}

static inline const kbinput_key	*_p1_toggle_pause(const kbinput_key *event) {
	pthread_mutex_lock(&_game.lock);
	switch (_game.state.paused & 0x1U) {
		case 0x0U:
			if (!_toggle_pause(server_info.sockets.p1))
				event = NULL;
			break ;
		case 0x1U:
			if (!_start(server_info.sockets.p1))
				event = NULL;
	}
	if (event != NULL)
		_game.state.paused ^= 0x1U;
	pthread_mutex_unlock(&_game.lock);
	return event;
}

static inline const kbinput_key	*_p2_toggle_pause(const kbinput_key *event) {
	pthread_mutex_lock(&_game.lock);
	switch (_game.state.paused & 0x2U) {
		case 0x0U:
			if (!_toggle_pause(server_info.sockets.p2))
				event = NULL;
			break ;
		case 0x2U:
			if (!_start(server_info.sockets.p2))
				event = NULL;
	}
	if (event != NULL)
		_game.state.paused ^= 0x2U;
	pthread_mutex_unlock(&_game.lock);
	return event;
}

static inline const kbinput_key	*_p1_quit(const kbinput_key *event) {
	return (_quit(server_info.sockets.p1)) ? event : NULL;
}

static inline const kbinput_key	*_p2_quit(const kbinput_key *event) {
	return (_quit(server_info.sockets.p2)) ? event : NULL;
}

static inline u8	_move_paddle(const i32 socket, const direction direction) {
	message	msg;

	msg = (message){
		.version = server_info.version,
		.type = MESSAGE_CLIENT_MOVE_PADDLE,
		.length = sizeof(msg_clt_move_paddle),
	};
	msg.body.move_paddle = (msg_clt_move_paddle){
		.direction = direction
	};
	return _send_msg(socket, &msg);
}

static inline u8	_toggle_pause(const i32 socket) {
	message	msg;

	msg = (message){
		.version = server_info.version,
		.type = MESSAGE_CLIENT_PAUSE,
		.length = 0
	};
	return _send_msg(socket, &msg);
}

static inline u8	_start(const i32 socket) {
	message	msg;

	msg = (message){
		.version = server_info.version,
		.type = MESSAGE_CLIENT_START,
		.length = 0
	};
	return _send_msg(socket, &msg);
}

static inline u8	_quit(const i32 socket) {
	message	msg;

	msg = (message){
		.version = server_info.version,
		.type = MESSAGE_CLIENT_QUIT,
		.length = 0
	};
	return _send_msg(socket, &msg);
}

static inline u8	_print_msg(const message_type msg, const u32 wait) {
	const char	*_msg[3];

	switch (msg) {
		case PLAYER1_WON:
			_msg[0] = _MSG_GAME_OVER;
			_msg[1] = _MSG_GAME_OVER_P1_WON;
			_msg[2] = NULL;
			break ;
		case PLAYER2_WON:
			_msg[0] = _MSG_GAME_OVER;
			_msg[1] = _MSG_GAME_OVER_P2_WON;
			_msg[2] = NULL;
			break ;
		case PLAYER1_QUIT:
			_msg[0] = _MSG_GAME_OVER;
			_msg[1] = _MSG_P1_QUIT;
			_msg[2] = NULL;
			break ;
		case PLAYER2_QUIT:
			_msg[0] = _MSG_GAME_OVER;
			_msg[1] = _MSG_P2_QUIT;
			_msg[2] = NULL;
			break ;
		case SERVER_CLOSED:
			_msg[0] = _MSG_GAME_OVER;
			_msg[1] = _MSG_SERVER_CLOSED;
			_msg[2] = NULL;
	}
	if (!display_msg(_msg))
		return 0;
	sleep(wait);
	return 1;
}

static inline void	_close(i32 fd) {
	if (fd >= 0)
		close(fd);
}

static inline u8	_init_connection(void) {
	message	msg;
	char	port[6];

	server_info.sockets.p1 = -1;
	server_info.sockets.p2 = -1;
	server_info.sockets.state = -1;
	if (!_connect(server_info.addr, server_info.port, &server_info.sockets.state))
		return 0;
	if (!_recv_msg(server_info.sockets.state, &msg) || msg.type != MESSAGE_SERVER_GAME_INIT)
		return 0;
	server_info.version = msg.version;
	utoa16(msg.body.init.p1_port, port);
	if (!_connect(server_info.addr, port, &server_info.sockets.p1))
		return 0;
	utoa16(msg.body.init.p2_port, port);
	if (!_connect(server_info.addr, port, &server_info.sockets.p2))
		return 0;
	if (!kb_io_listener.sigs.init) {
		if (sigemptyset(&_SIGS) == -1)
			return 0;
		if (!add_sig(_SIGS, SIGABRT) || !add_sig(_SIGS, SIGALRM) || !add_sig(_SIGS, SIGBUS) ||
			!add_sig(_SIGS, SIGFPE) || !add_sig(_SIGS, SIGHUP) || !add_sig(_SIGS, SIGILL) ||
			!add_sig(_SIGS, SIGINT) || !add_sig(_SIGS, SIGIO) || !add_sig(_SIGS, SIGPIPE) ||
			!add_sig(_SIGS, SIGPROF) || !add_sig(_SIGS, SIGPWR) || !add_sig(_SIGS, SIGQUIT) ||
			!add_sig(_SIGS, SIGSEGV) || !add_sig(_SIGS, SIGSTKFLT) || !add_sig(_SIGS, SIGSYS) ||
			!add_sig(_SIGS, SIGTERM) || !add_sig(_SIGS, SIGTRAP) || !add_sig(_SIGS, SIGUSR1) ||
			!add_sig(_SIGS, SIGUSR2) || !add_sig(_SIGS, SIGVTALRM) || !add_sig(_SIGS, SIGXCPU) ||
			!add_sig(_SIGS, SIGXFSZ) || !add_sig(_SIGS, SIGWINCH))
			return 0;
		kb_io_listener.sigs.init = 1;
	}
	pthread_mutex_lock(&kb_io_listener.start);
	return (pthread_create(&kb_io_listener.tid, NULL, _kb_io_listener, NULL) == 0) ? 1 : 0;
}

static inline u8	_connect(const char *addr, const char *port, i32 *sfd) {
	struct addrinfo	*addresses;
	struct addrinfo	*cur_address;

	if (getaddrinfo(addr, port, &hints, &addresses) != 0)
		return 0;
	for (cur_address = addresses; cur_address; cur_address = cur_address->ai_next) {
		*sfd = socket(cur_address->ai_family, cur_address->ai_socktype, cur_address->ai_protocol);
		if (*sfd) {
			if (connect(*sfd, cur_address->ai_addr, cur_address->ai_addrlen) != -1)
				break ;
			close(*sfd);
		}
	}
	freeaddrinfo(addresses);
	return (cur_address != NULL) ? 1 : 0;
}

static inline u8	_send(const i32 socket, const void *buf, const size_t n) {
	ssize_t	bytes_sent;
	size_t	total_sent;

	total_sent = 0;
	do {
		bytes_sent = send(socket, (const void *)((uintptr_t)buf + total_sent), n - total_sent, 0);
		total_sent += bytes_sent;
	} while (bytes_sent != -1 && total_sent < n);
	return (bytes_sent != -1) ? 1 : 0;
}

static inline u8	_recv(const i32 socket, void *buf, const size_t n) {
	ssize_t	bytes_read;
	size_t	total_read;

	total_read = 0;
	do {
		bytes_read = recv(socket, (void *)((uintptr_t)buf + total_read), n - total_read, MSG_WAITALL);
		total_read += bytes_read;
	} while (bytes_read > 0 && total_read < n);
	if (bytes_read == 0 && server_info.running)
		server_info.running = 0;
	return (bytes_read != -1 && total_read > 0) ? 1 : 0;
}

static inline u8	_send_msg(const i32 socket, const message *msg) {
	return _send(socket, msg, MESSAGE_HEADER_SIZE + msg->length);
}

static inline u8	_recv_msg(const i32 socket, message *msg) {
	if (!_recv(socket, msg, MESSAGE_HEADER_SIZE))
		return 0;
	switch (msg->type) {
		case MESSAGE_SERVER_GAME_INIT:
			return _recv(socket, &msg->body.init, sizeof(msg->body.init));
		case MESSAGE_SERVER_GAME_PAUSED:
			return 1;
		case MESSAGE_SERVER_GAME_OVER:
			if (server_info.version == 0)
				return _recv(socket, &msg->body.game_over.v0, sizeof(msg->body.game_over.v0));
			return _recv(socket, &msg->body.game_over.v1, sizeof(msg->body.game_over.v1));
		case MESSAGE_SERVER_STATE_UPDATE:
			return _recv(socket, &msg->body.state, sizeof(msg->body.state));
	}
	return 1;
}
