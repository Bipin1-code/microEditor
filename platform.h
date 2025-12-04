
#ifndef PLATFORM_H
#define PLATFORM_H

#include "keys.h"
#include <stdio.h>

void enableRawMode();
void disableRawMode();
int editorReadKey();
void editorRefreshScreen();
void initEditor();
void editorClearScreen();
void editorOpenFile(const char *fileName);
void editorAppendRow(const char *s, size_t len);
int getWindowSize(int *rows, int *cols);

#endif
