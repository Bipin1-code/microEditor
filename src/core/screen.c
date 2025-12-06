
#include <stdio.h>
#include <string.h>
#include "screen.h"
#include "editor.h"
#include "platform.h"

void editorDrawRows(eBuf *ab){
  for(int y = 0; y < E.screenrows; y++){
    int fileRow = y + E.rowOffset;
    
    if(fileRow >= E.numRows ){
      ebufAppend(ab, "~", 1);
    }else{
      int len = E.eRows[fileRow].size;
      if(len > E.screencols)
	len = E.screencols;
      ebufAppend(ab, E.eRows[fileRow].chars, len);
    }
    ebufAppend(ab, "\r\n", 2);
  }
}

void editorRefreshScreen(){
  eBuf ab = EDITORBUF_INIT;

  ebufAppend(&ab, "\x1b[?25l", 6);
  ebufAppend(&ab, "\x1b[H", 3);

  editorDrawRows(&ab);
    
  //position cursor
  char buf[32];
  snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
  ebufAppend(&ab, buf, strlen(buf));
  
  ebufAppend(&ab, "\x1b[?25h", 6);
 
  platformWrite(ab.b, ab.len);
  ebufFree(&ab);
}
