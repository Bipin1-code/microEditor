
#include "platform.h"
#include <stdio.h>

int main(int argc, char *argv[]){
  enableRawMode();
  initEditor();
  puts(" Press q for quit the Raw Mode.");

  if(argc >= 2){
    editorOpenFile(argv[1]);
  }

  while(1){
    editorRefreshScreen();
    int key = editorReadKey();

    /* if(key == ARROW_UP) */
    /*   puts("UP"); */
    /* else if(key == ARROW_DOWN) */
    /*   puts("DOWN"); */
    /* else if(key == ARROW_LEFT) */
    /*   puts("LEFT"); */
    /* else if(key == ARROW_RIGHT) */
    /*   puts("RIGHT"); */
    /* else */
    /*   printf("Key: %d ('%c')\n", key, key); */
    
    if(key == 'q'){
      editorClearScreen();
      break;
    }
  }
  return 0;
}
