#include "input.h"
#include "editor.h"
#include "platform.h"
#include <stdlib.h>

void editorProcessKeyPress() {
    int key = editorReadKey();

    if (key == 'q') {
        platformWrite("\x1b[2J\x1b[H", 7);
        disableRawMode();
        exit(0);
    }

    switch (key) {
        case ARROW_LEFT:
        case ARROW_RIGHT:
        case ARROW_UP:
        case ARROW_DOWN:
            editorMoveCursor(key);
            break;
    }
}
