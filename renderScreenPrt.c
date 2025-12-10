#include <stdio.h>
#include <windows.h>
#include <stdlib.h>

#define TAB 8
#define ARROW_UP    0x0101
#define ARROW_DOWN  0x0102
#define ARROW_LEFT  0x0103
#define ARROW_RIGHT 0x0104

static DWORD originalMode;

void disableRawMode(){
  SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), originalMode);
}

void enableRawMode(){
  HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
  GetConsoleMode(hin, &originalMode); //important
  
  DWORD raw = originalMode;

  raw &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);

  SetConsoleMode(hin, raw);

  atexit(disableRawMode);
}

int readKeyPress(){
  INPUT_RECORD record;
  DWORD count;
  while(1){
    ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &record, 1, &count);
    if(record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown){
      DWORD vk = record.Event.KeyEvent.wVirtualKeyCode;
      char c = record.Event.KeyEvent.uChar.AsciiChar;

      if(c != 0){
	return c;
      }
      switch(vk){
      case VK_UP     : return ARROW_UP;
      case VK_DOWN   : return ARROW_DOWN;
      case VK_LEFT   : return ARROW_LEFT;
      case VK_RIGHT  : return ARROW_RIGHT;
      case VK_RETURN : return '\r';   // Enter key
      case VK_BACK   : return 8;      // Backspace
      case VK_DELETE : return 127;    // Delete
      case VK_ESCAPE : return 27;     // Escape
      case VK_TAB    : return '\t';   // Tab
      }
    }
  }
}

//Editor
typedef struct{
  int size;
  int capacity;
  char *chars;
} EditorLine;

typedef struct{
  int fCx, fCy;
  int rowOffset, colOffset;

  int renderX;
  
  int screenRows, screenCols;

  int countOfL;
  int capOfL;
  EditorLine *lines;
} EditorState;

static EditorState E;

int getScreenSize(int *height, int *width){
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  if(!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)){
    return -1;
  }
  *width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
  *height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

  return 0;
}

void initEditor(){
  E.fCx = 0;
  E.fCy = 0;
  E.rowOffset = 0;
  E.colOffset = 0;
  E.renderX = 0;

  getScreenSize(&E.screenRows, &E.screenCols);

  E.countOfL = 0;
  E.capOfL = 16;
  E.lines = (EditorLine*)malloc(E.capOfL * sizeof(EditorLine));

  for (int i = 0; i < E.capOfL; i++) {
    E.lines[i].size = 0;
    E.lines[i].capacity = 0;
    E.lines[i].chars = NULL;
  }
}

/* -----softWrap helpers-----------*/
//how many visual rows a file line occupies
int visualRowsForLine(EditorLine *line){
  if(!line) return 1;
  if(line->size == 0) return 1;
  
  return (line->size + E.screenCols - 1) / E.screenCols;
}

//Total visual rows across whole file
int totalVisualRows(){
  int tot = 0;
  for(int i = 0; i < E.countOfL; i++){
    tot += visualRowsForLine(&E.lines[i]);
  }
  
  return tot;
}

//Given logical cursor for Y start from 0
int cursorVisualY(){
  int y = 0;
  for(int i = 0; i < E.fCy; i++){
    y += visualRowsForLine(&E.lines[i]);
  }
  y += (E.fCx / E.screenCols);

  return y;
}

//Given logical cursor return visual X (0-based)
int cursorVisualX(){
  return E.fCx % E.screenCols;
}

//Mapping visual coordinates (vy, vx) -- set logical E.fCy and E.fCx
//vx is desired visual x
void setCursorFromVisual(int vy, int vx){
  if(vy < 0) vy = 0;

  int acc = 0;
  for(int fileRow = 0; fileRow < E.countOfL; fileRow++){
    int vRows = visualRowsForLine(&E.lines[fileRow]);
    if(vy < acc + vRows){
      int part = vy - acc; 
      E.fCy = fileRow;
      int base = part * E.screenCols;
      //compute file line length
      int llen = E.lines[fileRow].size;
      int maxX = (llen - base > 0) ? (llen - base) : 0;
      
      if(vx > maxX)
	vx = maxX;
      
      E.fCx = base + vx;
      
      //clamp fCx to line length
      if(E.fCx > llen)
	E.fCx = llen;
      
      return;
    }
    acc += vRows;
  }
  //if we reach here, vy is beyond end of file-- set to last char at end
  if(E.countOfL == 0){
    E.fCy = 0;
    E.fCx = 0;
    return;
  }
  //last line
  int last = E.countOfL - 1;
  E.fCy = last;
  E.fCx = E.lines[last].size;
}

/* int cursorCxToRx(const char *s, int cx){ */
/*   int rx = 0; */
/*   for(int i = 0; i < cx; i++){ */
/*     if(s[i] == '\t'){ */
/*       rx += (TAB - (rx % TAB)); */
/*     }else{ */
/*       rx++; */
/*     } */
/*   } */
/*   return rx; */
/* } */

/* void renderScreen() { */
/*   printf("\x1b[2J"); */
/*   printf("\x1b[H"); */
/*   for (int y = 0; y < E.screenRows; y++) { */
/*     printf("\x1b[%d;1H", y + 1);  // Move cursor explicitly */
/*     int fileRow = y + E.rowOffset; */
/*     if (fileRow >= E.countOfL) { */
/*       printf("~"); */
/*     } else { */
/*       EditorLine *line = &E.lines[fileRow]; */
/*       int len = line->size; */
/*       int start = E.colOffset; */
/*       int end = E.colOffset + E.screenCols; */

/*       if (start > len) start = len; */
/*       if (end > len) end = len; */

/*       fwrite(&line->chars[start], 1, end - start, stdout); */
/*       printf("\x1b[K"); */
/*     } */
/*   } */
/* } */

void renderScreen(){

  printf("\x1b[?25l");
  printf("\x1b[H");
  printf("\x1b[2J");
  //how many visual rows we have printed so far
  int printed = 0;
  //running visual row index
  int visRow = 0;
  for(int fileRow = 0; fileRow < E.countOfL && printed < E.screenRows; fileRow++){
    EditorLine *line = &E.lines[fileRow];
    int len = line->size;
    int vRows = visualRowsForLine(line);
    
    for(int part = 0; part < vRows && printed < E.screenRows; part++){
      if(visRow < E.rowOffset){
	visRow++;
	continue;
      }

      int start = part * E.screenCols;
      int end = start + E.screenCols;

      if(start > len) start = len;
      if(end > len) end = len;
      int chunkLen = end - start;
      
      // Move to screen row (printed - rowOffset) + 1
      int screenY = printed + 1; //printed count from 0
      printf("\x1b[%d;1H", screenY);

      if(chunkLen > 0)
	fwrite(&line->chars[start], 1, chunkLen, stdout);

      if(chunkLen >= E.screenCols){
	printf("\x1b[1C");
	printf("\x1b[K");
      }else{
	printf("\x1b[K");
      }
      
      printed++;
      visRow++;
    }
  }
  //if there's remaining screen rows (after EOF),
  while(printed < E.screenRows){
    if(visRow >= E.rowOffset){
      printf("\x1b[%d;1H", printed + 1);
      printf("~");
      printf("\x1b[K");
      printed++;
    }
    visRow++;
    if(printed >= E.screenRows) break;
  }
  
  printf("\x1b[?25h");
  fflush(stdout);
}

void editorAppendLine(const char *s, size_t len){
  if(E.countOfL == E.capOfL){
    E.capOfL *= 2;
    E.lines = realloc(E.lines, E.capOfL * sizeof(EditorLine));
  }
  EditorLine *line = &E.lines[E.countOfL];
  line->size = (int)len;
  line->capacity = (int)len + 1;
  line->chars = malloc(line->capacity);
  memcpy(line->chars, s, len);
  line->chars[len] = '\0';

  E.countOfL++;
}

void editorOpenFile(const char *fileName){
  FILE *f = fopen(fileName, "rb");
  if(!f){
    printf("filed to load %s file", fileName);
    return;
  }

  char *line = NULL;
  size_t cap = 0;
  size_t len = 0;
  
  int c;
  while((c = fgetc(f)) != EOF){
    if(c == '\n'){
      if(len == 0)
	editorAppendLine("", 0);
      else
	editorAppendLine(line, len);
      
      len = 0;
    }else if(c == '\r'){
      continue;
    }else{
      if(len + 1 >= cap){
	cap = (cap == 0) ? 128 : cap * 2;
	line = realloc(line, cap);
      }
      line[len++] = (char)c;
    }
  }

  if(len > 0){
    editorAppendLine(line, len);
  }
  free(line);
  fclose(f);
}

void editorScroll(){
  int cVY = cursorVisualY();
  if(cVY < E.rowOffset){
    E.rowOffset = cVY;
  }
  else if(cVY >= E.rowOffset + E.screenRows){
    E.rowOffset = cVY - E.screenRows + 1;
  }

  if(E.fCx < E.colOffset){
    E.colOffset = E.fCx;
  }
  else if(E.fCx >= E.colOffset + E.screenCols){
    E.colOffset = E.fCx - E.screenCols + 1;
  }
}

void moveCursor(int key){
  switch(key){
  case ARROW_UP: {
    int cVY = cursorVisualY();
    if(cVY > 0){
      int cVX = cursorVisualX();
      cVY--;
      setCursorFromVisual(cVY, cVX);
    }
    break;
  }
  
  case ARROW_DOWN:{
    int cVY = cursorVisualY();
    int tot = totalVisualRows();
    if(cVY + 1 < tot){
      int cVX = cursorVisualX();
      cVY++;
      setCursorFromVisual(cVY, cVX);
    }
    break;
  }
  
  case ARROW_LEFT:{
    if(E.fCx > 0){
      E.fCx--;
    }else if(E.fCy > 0){
      E.fCy--;
      E.fCx = E.lines[E.fCy].size;
    }
    break;
  }
 
  case ARROW_RIGHT:{
    int len = (E.fCy < E.countOfL) ? E.lines[E.fCy].size : 0;
    if(E.fCx < len){
      E.fCx++;
    }else if(E.fCy + 1 < E.countOfL){
      E.fCy++;
      E.fCx = 0;
    }
    break;
  } 
  }
  //prevent cursor from going past end of line
  if(E.fCy >= E.countOfL)
    E.fCy = E.countOfL - 1;
  if(E.fCy < 0) {
    E.fCy = 0;
    E.fCx = 0;
  }
  int lineLen = (E.fCy < E.countOfL) ? E.lines[E.fCy].size : 0;
  if(E.fCx > lineLen)
    E.fCx = lineLen;
}

void positionCursor(){
  int cVY = cursorVisualY();
  int cVX = cursorVisualX();
  int screenY = cVY - E.rowOffset + 1;
  int screenX = cVX + 1;

  if(screenY < 1) screenY = 1;
  if(screenY > E.screenRows)
    screenY = E.screenRows;

  if(screenX < 1) screenX = 1;
  if(screenX > E.screenCols)
    screenX = E.screenCols;
  
  printf("\x1b[%d;%dH", screenY, screenX);
  fflush(stdout);
}

int main(int argc, char *argv[]){
  printf("Today I have to Conquered this rendering.\n");
  printf("\x1b[32;43mPress any key to begin>>>>\x1b[0m\n");
  getchar();

  enableRawMode();
  initEditor();

  if(argc >= 2){
    editorOpenFile(argv[1]);
  }
  
  while(1){
    editorScroll();
    renderScreen();
    positionCursor();
    int c = readKeyPress();
    
    if(c == 'q' || c == 'Q'){
      printf("\x1b[2J");
      break;
    }

    switch(c){
    case ARROW_UP:
    case ARROW_DOWN:
    case ARROW_LEFT:
    case ARROW_RIGHT:
      moveCursor(c);
      break;
    }
  }
  return 0;
}

