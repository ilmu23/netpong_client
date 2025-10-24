# netpong

![1.png](https://github.com/ilmu23/netpong_client/raw/master/img/1.png)

## Overview

netpong is a terminal client for the [pong server](https://github.com/ilmu23/netpong_server) I wrote.

## Compilation

0. Install [libkbinput](https://github.com/ilmu23/kbinput)
1. Clone the repository and run make
    ```bash
    git clone https://github.com/ilmu23/netpong_client
    cd netpong_client
    make
    ```
## Usage

Run the executable, passing the address and port of the server as arguments.

```bash
./netpong 127.0.0.1 4242
```

### Menu navigation

ACTION          |   KEYS
| :---:         |   :---:
Navigate up     |   k, w, Up-arrow
Navigate down   |   j, s, Down-arrow
Navigate left   |   h, a, Left-arrow
Navigate right  |   l, d, Right-arrow
Select          |   Enter
Back            |   Backspace
Quit            |   q, Escape

### Game controls

ACTION              |   KEY
| :---:             |   :---:
P1 move paddle up   |   w
P1 move paddle down |   s
P2 move paddle up   |   Up-arrow
P2 move paddle down |   Down-arrow
P1 toggle pause     |   p
P2 toggle pause     |   Shift + p
P1 quit             |   q
P2 quit             |   Shift + q

## Notes

If your terminal supports the [kitty keyboard protocol](https://sw.kovidgoyal.net/kitty/keyboard-protocol),
the paddles will only move while the corresponding keys are held down.
In the legacy mode the movement keys instead toggle movement in the
desired direction.
