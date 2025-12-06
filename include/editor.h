#ifndef EDITOR_H
#define EDITOR_H

#include "buffer.h"
#include <stddef.h>

//Define EditorRow
//Every line in the text file is stored in a struct
typedef struct {
  int size;  //number of characters in the row 
  char *chars; 
} editorRow;

typedef struct editorConfig{
  //cursor X and Y position
  int cx, cy; //cx: cursor column (X position) and cy: cursor row (Y position)
  
  
  int rowOffset; //Row offset for vertical scrolling
  //int colOffset;
  
  int screenrows; //these belongs to device's display on which editor is running
  int screencols;

  int numRows; //this keep track eRow (size of eRow array);
  editorRow *eRows;  //this represent editor 1 line (how many characters it has)
} editorConfig;

extern editorConfig E;

void initEditor();
void editorMoveCursor(int key);


//file I/O
/* void editorAppendRow(editorConfig *E, const char *s, size_t len); */

#endif
