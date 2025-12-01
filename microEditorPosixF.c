
#include "plateform.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>

#define EDITOR_VERSION "0.0.1"

//global config to store original terminal settings
struct termios orig_termios;


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
  if(c == "\x1b"){
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

void disableRawMode(){
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void editorClearScreen(){
  printf("\x1b[2J");
  printf("\x1b[H");
}

void editorDrawRows(){
  for(int y = 0; y < 24; y++){
    if(y == 1)
      printf("------Micro Editor (v%s)-----\r\n", EDITOR_VERSION);
    else
      printf("~\r\n");
  } 
}

void editorRefreshScreen(){
  editorClearScreen();
  editorDrawRows();
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

