
#include "platform.h"
#include <stdio.h>

int main(){
  enableRawMode();
  puts(" Press q for quit the Raw Mode.");

  while(1){
    char c = editorReadKey();
    printf("Key: %d ('%c')\n", c, c);
    if(c == 'q') break;
  }
  return 0;
}
