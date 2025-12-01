
#include "platform.h"
#include <stdio.h>

int main(){
  enableRawMode();
  initEditor();
  puts(" Press q for quit the Raw Mode.");

  while(1){
    editorRefreshScreen();
    int key = editorReadKey();
    if(key == ARROW_UP)
      puts("UP");
    else if(key == ARROW_DOWN)
      puts("DOWN");
    else if(key == ARROW_LEFT)
      puts("LEFT");
    else if(key == ARROW_RIGHT)
      puts("RIGHT");
    else
      printf("Key: %d ('%c')\n", key, key);
    
    if(key == 'q'){
      editorClearScreen();
      break;
  }
  return 0;
}

