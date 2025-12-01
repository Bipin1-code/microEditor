
#include "platform.h"
#include <windows.h>
#include <stdio.h>

#define EDITOR_VERSION "0.0.1"

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

int editorReadKey(){
  INPUT_RECORD record;
  DWORD count;
  while(1){
    ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &record, 1, &count);
  
    if(record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown){
      DWORD vk = record.Event.KeyEvent.wVirtualKeyCode;
      char c = record.Event.KeyEvent.uChar.AsciiChar;

      if(c != 0){
	return c; //Normal ASCII key
      }

      switch(vk){
         case VK_UP   : return ARROW_UP;
         case VK_DOWN : return ARROW_DOWN;
         case VK_LEFT : return ARROW_LEFT;
         case VK_RIGHT: return ARROW_RIGHT;
      }
    }
  }
}

struct editorConfig{
  int cx, cy; //cursor X and Y position
  int screenrows;
  int screencols;
};

struct editorConfig E;

void initEditor(){
  E.cx = 0;
  E.cy = 0;

  getWindowSize(&E.screenrows, &E.screencols);
}

int getWindowSize(int *rows, int *cols){
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  if(!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)){
    return -1;
  }

  *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
  *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

  return 0;
}

void editorClearScreen(){
  printf("\x1b[2J");
  printf("\x1b[H");
}

void editorDrawRows(){
  for(int y = 0; y < E.screenrows; y++){
    if(y == E.screenrows / 3){
      char welcome[80];
      int welcomelen = snprintf(welcome, sizeof(welcome), "Mcro Editor (v%s)", EDITOR_VERSION);

      if(welcomelen > E.screencols)
	welcomelen = E.screencols;

      int padding = (E.screencols - welcomelen) / 2;
      if(padding){
	printf("~");
	padding--;
      }

      while(padding--) printf(" ");

      printf("%.*s", welcomelen, welcome);
    }else{
      printf("~");
    }
    printf("\r\n");
  }
}

void editorRefreshScreen(){
  editorClearScreen();
  editorDrawRows();
  printf("\x1b[%d;%dH", E.cy + 1, E.cx + 1); // cursor position
  fflush(stdout);
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
