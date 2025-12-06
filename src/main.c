
#include "platform.h"
#include "editor.h"
#include "fileio.h"
#include "screen.h"
#include "input.h"

int main(int argc, char *argv[]){
  enableRawMode();
  initEditor();

  if(argc >= 2)
    editorLoadFile(argv[1]);

  while(1){
    editorRefreshScreen();
    editorProcessKeyPress();
  }
  
  return 0;
}
