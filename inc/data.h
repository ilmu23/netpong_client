// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<data.h>>

#pragma once

#include "defs.h"

#define MESSAGE_HEADER_SIZE	4

#define MESSAGE_CLIENT_START		0
#define MESSAGE_CLIENT_PAUSE		1
#define MESSAGE_CLIENT_MOVE_PADDLE	2
#define MESSAGE_CLIENT_QUIT			3

#define MESSAGE_SERVER_GAME_INIT	0
#define MESSAGE_SERVER_GAME_PAUSED	1
#define MESSAGE_SERVER_GAME_OVER	2
#define MESSAGE_SERVER_STATE_UPDATE	3

#define GAME_OVER_ACT_WON		0x1U
#define GAME_OVER_ACT_QUIT		0x2U
#define GAME_OVER_SERVER_CLOSED	0x3U

typedef struct [[gnu::packed]] {
	u16	p1_port;
	u16	p2_port;
}	msg_srv_init;

typedef struct [[gnu::packed]] {
	u8	winner_id;
}	msg_srv_game_over_v0;

typedef struct [[gnu::packed]] {
	u8	actor_id;
	u8	finish_status;
	u16	score;
}	msg_srv_game_over_v1;

typedef union [[gnu::packed]] {
	msg_srv_game_over_v0	v0;
	msg_srv_game_over_v1	v1;
}	msg_srv_game_over;

typedef struct [[gnu::packed]] __msg_srv_state {
	f32	p1_paddle;
	f32	p2_paddle;
	struct {
		f32	x;
		f32	y;
	}	ball;
	u16	score;
}	msg_srv_state;

typedef struct [[gnu::packed]] __msg_clt_move_paddle {
	u8	direction;
}	msg_clt_move_paddle;

typedef struct [[gnu::packed]] {
	u8	version;
	u8	type;
	u16	length;
	union [[gnu::packed]] {
		msg_srv_init		init;
		msg_srv_game_over	game_over;
		msg_srv_state		state;
		msg_clt_move_paddle	move_paddle;
	}	body;
}	message;
