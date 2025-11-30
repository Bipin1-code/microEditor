
#include "platform.h"
#include <windows.h>
#include <stdio.h>

DWORD originalMode;

void disableRawMode(){
  SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), originalMode);
}

void enableRawMode(){
  HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
  GetConsoleMode(hIn, &originalMode);

  DWORD raw = originalMode;
  raw &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);

  SetConsoleMode(hIn, raw);

  atexit(disableRawMode);
}

char editorReadKey(){
  INPUT_RECORD record;
  DWORD count;
  while(1){
    ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &record, 1, &count);
  
    if(record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown){
      char c = record.Event.KeyEvent.uChar.AsciiChar;
      return c;
    }
  }
}
/* int main(){ */
/*   printf("Micro Editor Development session:\n"); */
/*   enableRawMode(); */
/*   printf("Raw mode enabled. Press 'q' to quit.\n"); */

/*   while(1){ */
/*     char c = editorReadKey(); */
/*     printf("Key: %d ('%c')\n", c, c); */
/*     if(c == 'q') break; */
/*   } */
/*   return 0; */
/* } */
