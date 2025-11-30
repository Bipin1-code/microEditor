
#include "platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <ctype.h>


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

char editorReadKey(){
  char c;
  while(read(STDIN_FILENO, &c, 1) != 1);
  return c;
}

void disableRawMode(){
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
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
