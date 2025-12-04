
#include "platform.h"
#include <windows.h>
#include <stdio.h>

#define EDITOR_VERSION "0.0.1"

/*
  ssize_t: A signed integer type used for sizes and byte counts,
  capable of holding −1 for errors and large values on 64-bit systems.
  Often used as the return type of POSIX functions like read() and write().
  In windows it is define inside BaseTsd.h -> WinDef.h -> Windows.h
 */
//typedef SSIZE_T ssize_t; 

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

//Define EditorRow
//Every line in the text file is stored in a struct
typedef struct editorRow{
  int size;  //number of characters in the row 
  char *chars; 
} editorRow;

struct editorConfig{
  int cx, cy; //cursor X and Y position
  int screenrows;
  int screencols;

  int numRows;
  editorRow *eRow;
};

struct editorConfig E;

void initEditor(){
  E.cx = 0;
  E.cy = 0;
  E.numRows = 0;
  E.eRow = NULL;
  
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

//this function takes a line from a file and stores it inside E.rRow
void editorAppendRow(const char *s, size_t len){
  E.eRow = realloc(E.eRow, sizeof(editorRow) * (E.numRows + 1));

  int at = E.numRows;
  E.eRow[at].size = len;
  E.eRow[at].chars = malloc(len + 1);
  memcpy(E.eRow[at].chars, s, len);
  E.eRow[at].chars[len] = '\0';
  E.numRows++;
}

//this function load the file in editor
void editorOpenFile(const char *fileName){
  FILE *f = fopen(fileName, "r");
  if(!f) return;

  char *line = NULL;
  size_t cap = 0;
  size_t len = 0;

  int c;
  
  while((c = fgetc(f)) != EOF){
    if(len + 1 >= cap){
      cap = (cap == 0) ? 128 : (cap * 2);
      line = realloc(line, cap);
    }
    if(c == '\n'){
      //remove \n and store the row
      editorAppendRow(line, len);
      len = 0;
    }else if(c == '\r'){
      //ignore CR (WINDOWs CRLF)
      continue;
    }else{
      line[len++] = c;
    }
  }
  //last line if file does not end with newline
  if(len > 0){
    editorAppendRow(line, len);
  }

  free(line);
  fclose(f);
}

void editorClearScreen(){
  //printf("\x1b[?25l");//hide cursor
  printf("\x1b[2J"); //clear screen
  printf("\x1b[H"); //move cursor to home (top-left)
}

void editorDrawRows(){
  for(int y = 0; y < E.screenrows; y++){
    if(y < E.numRows ){
      int len = E.eRow[y].size;
      if(len > E.screencols)
	len = E.screencols;
      printf("%.*s", len, E.eRow[y].chars);
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
  printf("\x1b[?25h");
  fflush(stdout);
}


//Main for independent test 
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
