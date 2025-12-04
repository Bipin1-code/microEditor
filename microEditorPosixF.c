
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <string.h>

#define EDITOR_VERSION "0.0.1"

//global config to store original terminal settings
struct termios orig_termios;

void disableRawMode(){
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode(){
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disableRawMode);

  struct termios raw = orig_termios;
  //Disable canonical mode and echo
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  //Disable C-S and C-Q, disable CR->NewLine translation
  raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
  //Disable post-processing of output
  raw.c_oflag &= ~(OPOST);
  //Control setting (8-bit chars)
  raw.c_cflag |= (CS8);
  //Minimum bytes for read = 0
  raw.c_cc[VMIN] = 0;
  //Timeout for read = 1 (0.1 sec)
  raw.c_cc[VTIME] = 1;

  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int editorReadKey(){
  char c;
  size_t n;
  while((n = read(STDIN_FILENO, &c, 1)) != 1);

  //if character is CSI (Control Sequence Introducer) which is 27 (in hex it's 0x1b)
  //'\x_' means- Insert the character whose ASCII Value is this hex number(_).
  if(c == '\x1b'){
    char seq[3];

    //read next two bytes if available
    if(read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
    if(read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

    if(seq[0] == '['){
      switch(seq[1]){
        case 'A': return ARROW_UP;
        case 'B': return ARROW_DOWN;
        case 'C': return ARROW_RIGHT;
        case 'D': return ARROW_LEFT;
      }
    }
    return '\x1b';
  }
  return c;
}

typedef struct{
  int size;
  char *chars;
} editorRow;

struct editorConfig{
  int cx, cy;
  int screenrows;
  int screencols;

  int numRows;
  editorRow *eRows;
};

struct editorConfig E;

int getWindowSize(int *rows, int *cols){
  struct winsize ws; 

  if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1){
    return -1;
  }
  *cols = ws.ws_col;
  *rows = ws.ws_row;
  
  return 0;
}

void initEditor(){
  E.cx = 0;
  E.cy = 0;
  E.numRows = 0;
  E.eRows = NULL;

  getWindowSize(&E.screenrows, &E.screencols);
}

void editorAppendRow(const char *s, size_t len){
  E.eRows = realloc(E.eRows, sizeof(editorRow) * (E.numRows + 1));

  int at =  E.numRows;
  E.eRows[at].size = len;
  E.eRows[at].chars = malloc(len+1);
  memcpy(E.eRows[at].chars, s, len);
  E.eRows[at].chars[len] = '\0';

  E.numRows++;
}

//Precaution: This function may remove or update in future
int isBinaryFile(FILE *f){
  unsigned char buf[4096];
  size_t n = fread(buf, 1, sizeof(buf), f);
  rewind(f);

  if(n == 0)
    return 0; //empty file

  for(size_t i = 0; i < n; i++){
    unsigned char c = buf[i];

    if(c == 0) return 1; //NULL byte ->definitely binary 
    if(c < 9) return 1; //weird control characters
    if(c > 126){
      if(!(c == '\n' || c == '\r' || c == '\t'))
	return 1;
    }
  }
  return 0; 
}

void editorOpenFile(const char *fileName){
  FILE *f = fopen(fileName,"r");
  if(!f){
    printf("Cannot open file: %s\n", fileName);
    return;
  }

  if(isBinaryFile(f)){
    printf("Error: '%s' looks like a binary file. Cannot open.\n", fileName);
    fclose(f);
    return;
  }

  char *line = NULL;
  size_t cap = 0;
  ssize_t linelen;

  while((linelen = getline(&line, &cap, f)) != -1){
    while(linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) //we use while because \r\n may came together.(POSIX it always)
      linelen--; //this remove '\n' and '\r'

    editorAppendRow(line, linelen);
  }
  
  free(line);
  fclose(f);
}

void editorClearScreen(){
  printf("\x1b[2J");
  printf("\x1b[H");
}

void editorDrawRows(){
  for(int y = 0; y < E.screenrows; y++){
    if(y < E.numRows){
      int len = E.eRows[y].size;
      if(len > E.screencols)
	len = E.screencols;
      printf("%.*s", len, E.eRows[y].chars);
    }else{
      printf("~");
    }
    printf("\r\n"); 
  } 
}

void editorRefreshScreen(){
  editorClearScreen();
  editorDrawRows();
  printf("\x1b[%d;%dH", E.cy + 1, E.cx + 1);
  printf("\x1b[?25h");
  fflush(stdout);
}


/* int main(){ */
/*   printf("Micro Editor Development session:\n"); */

/*   enableRawMode(); */
/*   printf("Raw mode enabled. Press 'q' to quit.\n"); */

/*   while(1){ */
/*     char c = editorReadKey(); */

/*     if(iscntrl(c)) */
/*       printf("%d\r\n", c); */
/*     else */
/*       printf("%d ('%c')\r\n", c,c); */

/*     if(c == 'q') break; */
/*   } */
  
/*   return 0; */
/* } */
