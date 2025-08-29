// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<menu.h>>

#pragma once

#include "defs.h"

typedef struct __menu_item	menu_item;
typedef struct __menu		menu;

typedef enum __menu_action {
	PLAY,
	LOGIN,
	ENTER_MENU,
	SELECT_OPTION,
	BACK,
	EXIT
}	menu_action;

typedef void	(*opt_setter)(const uintptr_t);

struct __menu_item {
	struct {
		menu_item	*up;
		menu_item	*down;
		menu_item	*left;
		menu_item	*right;
	}			neighbors;
	struct {
		opt_setter	set;
		uintptr_t	val;
	}			option;
	const char	*title;
	const menu	*next;
	const menu	*prev;
	menu_action	action;
	u8			selected;
};

struct __menu {
	menu_item	*root;
	menu_item	*current;
	size_t		longest_title;
	u8			height;
	u8			width;
};

u8	main_menu(const char *server_addr, const char *server_port);

void	cleanup(void);
