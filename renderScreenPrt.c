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
  int preferredRx; //added for to fix softwrap cursor problem
  
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
  E.preferredRx = 0;

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

//effective columns we render (leave last column unused)
static int effectiveCols(){
  int c = E.screenCols - 1;
  if(c < 1) return 1;
  return c;
}

/* ---------- Tab-aware helpers (rx) ---------- */
/*
 rx_from_cx(line, cx) -> returns rendered column (rx) for logical index cx.
 It counts characters and expands '\t' to tab stops (TAB).
 cx may be from 0..line->size.
*/

int rx_from_cx(EditorLine *line, int cx){
  if(!line) return 0;
  int rx = 0;
  int limit = cx;
  if(limit > line->size) limit = line->size;
  
  for(int i = 0; i < limit; i++){
    unsigned char ch = (unsigned char)line->chars[i];
    if(ch == '\t'){
      rx += (TAB - (rx % TAB));
    }else{
      rx++;
    }
  }
  return rx;
}

/*
 cx_from_rx(line, targetRx) -> returns the largest cx such that rx_from_cx(cx) <= targetRx.
 Useful to map a visual x back to logical cx.
*/
int cx_from_rx(EditorLine *line,  int targetRx){
  if(!line) return 0;
  if(targetRx <= 0) return 0;
  int rx = 0;

  for(int i = 0; i < line->size; i++){
    unsigned char ch = (unsigned char)line->chars[i];
    if(ch == '\t'){
      int add = TAB - (rx % TAB);
      if(rx + add > targetRx) return i;
      rx += add;
    }else{
      if(rx + 1 > targetRx) return i;
      rx++;
    }
  }

  //targetRx may be beyond end-> return end
  return line->size;
}

//renderedLen(line) ->total rendered cols for the whole line (rx length).
int renderedLen(EditorLine *line){
  if(!line) return 0;
  return rx_from_cx(line, line->size);
}

/* -----softWrap helpers-----------*/

//how many visual rows a file line occupies
int visualRowsForLine(EditorLine *line){
  int eff = effectiveCols();
  int rLen = renderedLen(line);
  if(rLen == 0) return 1;
  
  return (rLen + eff - 1) / eff;
}

//Total visual rows across whole file
int totalVisualRows(){
  int tot = 0;
  for(int i = 0; i < E.countOfL; i++){
    tot += visualRowsForLine(&E.lines[i]);
  }
  
  return tot;
}

//sum rendered rows of previous lines + current line's rendered rx / eff
int cursorVisualY(){
  int eff = effectiveCols();
  int y = 0;
  
  for(int i = 0; i < E.fCy; i++){
    y += visualRowsForLine(&E.lines[i]);
  }
  
  int rx = rx_from_cx(&E.lines[E.fCy], E.fCx);
  y += (rx / eff);

  return y;
}

//cursor  visual X (0-based) = rx % eff
int cursorVisualXLocal(){
  int eff = effectiveCols();
  int rx = rx_from_cx(&E.lines[E.fCy], E.fCx);
  
  return rx % eff;
}

//cursor  visual X (0-based)
int cursorVisualXAbsolute(){
  return rx_from_cx(&E.lines[E.fCy], E.fCx);
}

/*
  Mapping visual coordinates (vy, vx):
  -find which file line contains visual row vy
  -compute the wrapped chunk baseRx and map vx->cx using cx_from_rx
*/
void setCursorFromVisual(int vy, int vxAbsolute){
  if(vy < 0) vy = 0;
  int acc = 0;
  int eff = effectiveCols();
  
  for(int fileRow = 0; fileRow < E.countOfL; fileRow++){
    int vRows = visualRowsForLine(&E.lines[fileRow]);
    if(vy < acc + vRows){
      int part = vy - acc; 
      E.fCy = fileRow;
      int baseRx = part * eff;
      int rlineLen = renderedLen(&E.lines[fileRow]);
      int localPref = vxAbsolute % eff;
      int targetRx = baseRx + localPref;
      
      if(targetRx > rlineLen) targetRx = rlineLen;
      if(targetRx < 0) targetRx = 0;
      
      //compute the corresposnding cx for this targetRx
      int cx = cx_from_rx(&E.lines[fileRow], targetRx);
      //ensure cx not beyond line length
      if(cx > E.lines[fileRow].size)
	cx = E.lines[fileRow].size;
      
      E.fCx = cx;
      
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
  E.fCy = E.countOfL - 1;
  E.fCx = E.lines[E.fCy].size;
}

void renderScreen(){

  int eff = effectiveCols();
  printf("\x1b[?25l");
  printf("\x1b[H");
  printf("\x1b[2J");
  //how many visual rows we have printed so far
  int printed = 0;
  //running visual row index
  int visRow = 0;
  for(int fileRow = 0; fileRow < E.countOfL && printed < E.screenRows; fileRow++){
    EditorLine *line = &E.lines[fileRow];
    int rlineLen = renderedLen(line);
    int vRows = visualRowsForLine(line);
    
    for(int part = 0; part < vRows && printed < E.screenRows; part++){
      if(visRow < E.rowOffset){
	visRow++;
	continue;
      }

      int baseRx = part * eff;
      int chunkRx = eff;

      if(baseRx + chunkRx > rlineLen){
	chunkRx = (rlineLen > baseRx) ? (rlineLen - baseRx) : 0;
      }

      /* -We need to extract the substring (logical chars)
	 that correspond to rx range [baseRx, baseRx + chunkRx]
	 -Build a small buffer by walking logical chars and
	 emitting their expanded form until we cover needed rx range.
	 -We'll write only the needed part to screen.
      */
      char outbuf[1024];
      int outlen = 0;
      int rx = 0;
      int i = 0;
      // walk logical chars, stop when rx >= base_rx + chunk_rx
      while(i < line->size && rx < baseRx + chunkRx){
	unsigned char ch = (unsigned char)line->chars[i];
	if(ch == '\t'){
	  int add = TAB - (rx % TAB);
	  if(rx + add <= baseRx){
	    rx += add;
	    i++;
	    continue;
	  }
	  for(int s = 0; s < add && rx < baseRx + chunkRx; s++){
	    if(rx >= baseRx){
	      if(outlen < (int)(sizeof(outbuf))) outbuf[outlen++] = ' ';
	    }
	    rx++;
	  }
	  i++;
	}else{
	  if(rx >= baseRx && rx < baseRx + chunkRx){
	    if(outlen < (int)(sizeof(outbuf)))
	      outbuf[outlen++] = (char)ch;
	  }
	  rx++;
	  i++;
	}
      }
      
      // Move to screen row (printed - rowOffset) + 1
      int screenY = printed + 1; //printed count from 0
      printf("\x1b[%d;1H", screenY);

      if(outlen > 0)
	fwrite(outbuf, 1, outlen, stdout);
      
      printf("\x1b[K");

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

/* ---------- File loading / append ---------- */
void editorAppendLine(const char *s, size_t len){
  if(E.countOfL == E.capOfL){
    E.capOfL *= 2;
    E.lines = realloc(E.lines, E.capOfL * sizeof(EditorLine));
  }
  EditorLine *line = &E.lines[E.countOfL];
  line->size = (int)len;
  line->capacity = (line->size < 64) ? 64 : line->size + 1;
  line->chars = malloc(line->capacity);
  if(len > 0)
    memcpy(line->chars, s, len);
  
  line->chars[len] = '\0';
  E.countOfL++;
}

void editorOpenFile(const char *fileName){
  FILE *f = fopen(fileName, "rb");
  if(!f){
    fprintf(stderr, "filed to load %s file.\n", fileName);
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

  //no horizontal scroll with soft-wrap
  E.colOffset = 0;
  /* if(E.fCx < E.colOffset){ */
  /*   E.colOffset = E.fCx; */
  /* } */
  /* else if(E.fCx >= E.colOffset + E.screenCols){ */
  /*   E.colOffset = E.fCx - E.screenCols + 1; */
  /* } */
}

/*-------------key Processing----------------*/
void moveCursor(int key){
  switch(key){
  case ARROW_UP: {
    int cVY = cursorVisualY();
    if(cVY > 0){
      /* int cVX = cursorVisualXLocal(); */
      cVY--;
      setCursorFromVisual(cVY, E.preferredRx);
    }
    break;
  }
  
  case ARROW_DOWN:{
    int cVY = cursorVisualY();
    int tot = totalVisualRows();
    if(cVY + 1 < tot){
      /* int cVX = cursorVisualXLocal(); */
      cVY++;
      setCursorFromVisual(cVY, E.preferredRx);
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
    E.preferredRx = cursorVisualXAbsolute();
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
    E.preferredRx = cursorVisualXAbsolute();
    break;
  } 
  }
  //prevent cursor from going past end of line
  if(E.fCy >= E.countOfL)
    E.fCy = (E.countOfL ? E.countOfL - 1 : 0);
  
  if(E.fCy < 0) {
    E.fCy = 0;
    E.fCx = 0;
  }
  //Clamp cursor to valid char position in current line
  int lineLen = (E.fCy < E.countOfL) ? E.lines[E.fCy].size : 0;
  if(E.fCx > lineLen)
    E.fCx = lineLen;
}

void positionCursor(){
  int cVY = cursorVisualY();
  int cVX = cursorVisualXLocal();
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

/*---------Editor Operation----------------- */
void ensureLineCapacity(EditorLine *line, int newSize){
  if(newSize + 1 > line->capacity){
    int newCap = line->capacity * 2;
    if(newCap < newSize + 1)
      newCap = newSize + 1;

    line->chars = realloc(line->chars, newCap);
    line->capacity = newCap;
  }
}

void editorInsertChar(int c){
  if(E.fCy >= E.countOfL) return;
  EditorLine *line = &E.lines[E.fCy];
  //expand capacity if needed
  ensureLineCapacity(line, line->size + 1);
  //shift right
  memmove(&line->chars[E.fCx + 1], &line->chars[E.fCx], line->size - E.fCx + 1);
  //insert new char
  line->chars[E.fCx] = (char)c;
  //Update size, cursor
  line->size++;
  E.fCx++;

  //update preferredRx for current arrow-up/down behavior
  E.preferredRx = cursorVisualXAbsolute();
}

//Backspace key operation
void editorDelCharInLine(EditorLine *line, int at){
  if(at < 0 || at >= line->size) return;
  memmove(&line->chars[at], &line->chars[at + 1], line->size - at);
  line->size--;
}

void editorJoinLineWithPrev(int row){
  if(row <= 0 || row >= E.countOfL) return;
  EditorLine *line = &E.lines[row];
  EditorLine *prev = &E.lines[row - 1];

  //grow previous line to hold new chars
  int newSize = prev->size + line->size;
  ensureLineCapacity(prev, newSize);

  //append current line to previous
  memcpy(&prev->chars[prev->size], line->chars, line->size);
  prev->size = newSize;
  prev->chars[newSize] = '\0';

  //remove current line from array
  free(line->chars);

  memmove(&E.lines[row], &E.lines[row + 1],
	  (E.countOfL - row - 1) * sizeof(EditorLine));

  E.countOfL--;
}

void editorBackspace(){
  if(E.fCy < 0 || E.fCy >= E.countOfL) return;

  //case 1: delete inside line
  if(E.fCx > 0){
    editorDelCharInLine(&E.lines[E.fCy], E.fCx - 1);
    E.fCx--;
    E.preferredRx = cursorVisualXAbsolute();
    
    return;
  }

  //case 2: starting of line -> join with previous line
  if(E.fCy > 0 && E.fCx == 0){
    int prevLen = E.lines[E.fCy - 1].size;
    editorJoinLineWithPrev(E.fCy);

    E.fCy--;
    E.fCx = prevLen;
    E.preferredRx = cursorVisualXAbsolute();
    
    return;
  }
}

//Delete key Operation
void editorDelRow(int at){
  if(at < 0 || at > E.countOfL) return;

  free(E.lines[at].chars);

  memmove(&E.lines[at], &E.lines[at + 1],
	  (E.countOfL - at - 1) * sizeof(EditorLine));

  E.countOfL--;
}

void editorDelChar(){
  if(E.fCy >= E.countOfL) return;

  EditorLine *line = &E.lines[E.fCy];

  //Case 1: delete character to the right
  if(E.fCx < line->size){
    //shift everything left by one
    memmove(&line->chars[E.fCx], &line->chars[E.fCx + 1],
	    line->size - (E.fCx - 1));
    line->size--;
    line->chars[line->size] = '\0';

    return;
  }

  //case 2: at end of line - merge with next line
  if(E.fCy + 1 < E.countOfL){
    EditorLine *nextLine = &E.lines[E.fCy + 1];

    //grow current row for next line contents
    line->chars = realloc(line->chars, line->size + (nextLine->size + 1));
    memcpy(&line->chars[line->size], nextLine->chars, nextLine->size);

    line->size += nextLine->size;
    line->chars[line->size] = '\0';

    //delete next line from array
    editorDelRow(E.fCy + 1);

    return;
  }
}

//Enter key Operation
void insertLine(int at, const char *s){
  E.lines = realloc(E.lines, (E.countOfL + 1) * sizeof(EditorLine));
  memmove(&E.lines[at + 1], &E.lines[at],
	  (E.countOfL - at) * sizeof(EditorLine));
  E.lines[at].size = strlen(s);
  E.lines[at].chars = strdup(s);
  E.countOfL++;
}

void editorInsertNewLine(){
  EditorLine *line = &E.lines[E.fCy];

  if(E.fCx == 0){
    //cursor at start: create an empty line above
    insertLine(E.fCy, "");
  }else{
    //split into two lines
    char *right = strdup(&line->chars[E.fCx]);
    //left part kept as current line
    line->size = E.fCx;
    line->chars[line->size] = '\0';

    insertLine(E.fCy + 1, right);

    free(right);
  }
  //Move the cursor to start of new line
  E.fCy++;
  E.fCx = 0;

  E.preferredRx = cursorVisualXAbsolute();
}

//Tab key (we define Tab = 8chars)
void editorProcessTab(){
  int rx = rx_from_cx(&E.lines[E.fCy], E.fCx);
  int spaces = TAB - (rx % TAB);

  for(int t = 0; t < spaces; t++){
    editorInsertChar(' ');
  }
}

/*---------------Main-----------------------*/
int main(int argc, char *argv[]){
  printf("Today I have to Conquered this rendering.\n");
  printf("\x1b[37;44mPress any key to begin>>>>\x1b[0m\n");
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
    }else if(c >= 32 && c <= 126){
      editorInsertChar(c);
    }

    if(c == 8){
      editorBackspace();
    }
    if(c == 127){
      editorDelChar();
    }
    if(c == '\r'){
      editorInsertNewLine();
    }
    if(c == '\t'){
      editorProcessTab();
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

