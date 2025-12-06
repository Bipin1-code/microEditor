
//editor.c logic comes in this file

#include "platform.h"
#include "editor.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

editorConfig E;

void initEditor(){
  E.cx = 0;
  E.cy = 0;
  E.rowOffset = 0;
  E.numRows = 0;
  E.eRows = NULL;
  
  getWindowSize(&E.screenrows, &E.screencols);
}


//this function updates cursor position according to x and y;
void editorMoveCursor(int key){
  switch(key){
  case ARROW_LEFT:
    if(E.cx > 0){
      E.cx--;
    }
    break;

  case ARROW_RIGHT:
    if(E.cx < E.screencols - 1){
      E.cx++;
    }
    break;
    
  case ARROW_UP:
    if(E.cy > 0){
      E.cy--;
    }
    break;
    
  case ARROW_DOWN:
    if(E.cy < E.numRows - 1){
      E.cy++;
    }
    break;
  }

  //Scroll up if cursor is above visible window
  if(E.cy < E.rowOffset){
    E.rowOffset = E.cy;
  }
  //scroll down if cursor is below visible window
  else if(E.cy >= E.rowOffset + E.screenrows){
    E.rowOffset = E.cy - E.screenrows + 1;
  }
}

