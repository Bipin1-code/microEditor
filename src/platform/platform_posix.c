//platform posix logic comes here

#include "platform.h"
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <string.h>

//global config to store original terminal settings
static struct termios orig_termios;

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


int getWindowSize(int *rows, int *cols){
  struct winsize ws; 

  if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0){
    if(ws.ws_col != 0 && ws.ws_row != 0){
      *cols = ws.ws_col;
      *rows = ws.ws_row;
      return 0;
    }
  }

  //fallback method
  char buf[32];
  unsigned int i = 0;

  //Move cursor bottom-right
  write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12);

  //Request cursor position
  write(STDOUT_FILENO, "\x1b[6n", 4);

  while(i < sizeof(buf) - 1){
    if(read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if(buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';

  //Parse the response: ESC [ rows; cols R
  if(buf[0] != "\x1b" || buf[1] != '[') return -1;

  if(sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

  return 0;
}


void platformWrite(const char *s, int len){
  write(STDOUT_FILENO, s, len);
}


/* int isBinaryFile(FILE *f){ */
/*   unsigned char buf[4096]; */
/*   size_t n = fread(buf, 1, sizeof(buf), f); */
/*   rewind(f); */

/*   if(n == 0) */
/*     return 0; //empty file */

/*   for(size_t i = 0; i < n; i++){ */
/*     unsigned char c = buf[i]; */

/*     if(c == 0) return 1; //NULL byte ->definitely binary  */
/*     if(c < 9) return 1; //weird control characters */
/*     if(c > 126){ */
/*       if(!(c == '\n' || c == '\r' || c == '\t')) */
/* 	return 1; */
/*     } */
/*   } */
/*   return 0;  */
/* } */


/* int platformOpenFile(const char *fileName, editorConfig *E){ */
/*   FILE *f = fopen(fileName,"r"); */
/*   if(!f){ */
/*     printf("Cannot open file: %s\n", fileName); */
/*     return -1; */
/*   } */

/*   if(isBinaryFile(f)){ */
/*     printf("Error: '%s' looks like a binary file. Cannot open.\n", fileName); */
/*     fclose(f); */
/*     return -1; */
/*   } */

/*   char *line = NULL; */
/*   size_t cap = 0; */
/*   ssize_t linelen; */

/*   while((linelen = getline(&line, &cap, f)) != -1){ */
/*     while(linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) //we use while because \r\n may came together.(POSIX it always) */
/*       linelen--; //this remove '\n' and '\r' */

/*     editorAppendRow(E, line, linelen); */
/*   } */
  
/*   free(line); */
/*   fclose(f); */

/*   return 0; */
/* } */
