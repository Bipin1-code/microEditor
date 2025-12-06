#ifndef PLATFORM_H
#define PLATFORM_H

#include "keys.h"

//Raw mode handler function
void enableRawMode();
void disableRawMode();

//key Input
int editorReadKey();

//Window
int getWindowSize(int *rows, int *cols);

//Cross-platform Write
void platformWrite(const char *s, int len);

/* void platformOpenFile(const char *fileName, editorConfig *E); */

#endif
