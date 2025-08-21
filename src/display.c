// ███████╗████████╗     ██████╗ ██╗   ██╗████████╗ ██████╗██╗  ██╗ █████╗ ██████╗
// ██╔════╝╚══██╔══╝     ██╔══██╗██║   ██║╚══██╔══╝██╔════╝██║  ██║██╔══██╗██╔══██╗
// █████╗     ██║        ██████╔╝██║   ██║   ██║   ██║     ███████║███████║██████╔╝
// ██╔══╝     ██║        ██╔═══╝ ██║   ██║   ██║   ██║     ██╔══██║██╔══██║██╔══██╗
// ██║        ██║███████╗██║     ╚██████╔╝   ██║   ╚██████╗██║  ██║██║  ██║██║  ██║
// ╚═╝        ╚═╝╚══════╝╚═╝      ╚═════╝    ╚═╝    ╚═════╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝
//
// <<display.c>>

#include <stdio.h>

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

u8	display_menu(const menu *menu) {
	const menu_item	*current;
	const menu_item	*down;

	if (fputs("\x1b[H\x1b[J", stdout) == EOF)
		return 0;
	for (current = menu->root; current; current = down) {
		for (down = current->neighbors.down; current; current = current->neighbors.right) {
			if (current->selected) {
				if (set_color_fg(colors.selection.fg) == -1 || set_color_bg(colors.selection.bg) == -1)
					return 0;
			} else if (set_color_fg(colors.fg) == -1)
				return 0;
			if (fprintf(stdout, " %s \x1b[m", current->title) == -1)
				return 0;
		}
		if (fputc('\n', stdout) == EOF)
			return 0;
	}
	return fflush(stdout) != EOF;
}
