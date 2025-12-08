
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#define ARROW_UP 0x0101
#define ARROW_DOWN 0x0102
#define ARROW_LEFT 0x0103
#define ARROW_RIGHT 0x0104
#define CTRL_Q 0x0202

typedef struct{
  int size;
  int capacity;
  char *chars;
} EditorLine;

typedef struct{
  int cursorX;
  int cursorY;
  
  int screenRows;
  int screenCols;

  //Scrolling offsets
  int rowOffset;  //which row is at top of screen
  int colOffset;  //which column is at left of screen

  int lineCount;
  int lineCapacity;
  EditorLine *lines;

  int mode;
} EditorState;

void disableTRawMode();
void enableTRawMode();
void enableANSI();
int readKeyPress();
void clearTScreen();

//Line operations:
void initLine(EditorLine *line);
void lineAppend(EditorLine *line, char c);
void lineInsert(EditorLine *line, int at, char c);
void lineDelete(EditorLine *line, int at);
void lineFree(EditorLine *line);

//Editor State Management:
void initEditor(EditorState *E);
void editorInsertLine(EditorState *E, int at, const char *text);
void editorFree(EditorState *E);

//Cursor and Screen Coordinate Conversion:
void editorCursorToScreen(EditorState *E, int *screenX, int *screenY);
void editorScroll(EditorState *E);

//Editing operation
void editorInsertChar(EditorState *E, char c);
void editorDeleteChar(EditorState *E);

//Rendering
void editorDrawScreen(EditorState *E);

int main(){
  printf("This is practice and debug session for screen rendering.\n");
  printf("Press any key to start...\n");
  getchar();

  EditorState E;
  initEditor(&E);
 
  // Load some initial content
  editorInsertLine(&E, 0, "Hello, World!");
  editorInsertLine(&E, 1, "This is a test.");
  editorInsertLine(&E, 2, "Use arrow keys to move.");

  enableANSI();
  enableTRawMode();
  
  while(1){
    editorDrawScreen(&E);
    
    int c = readKeyPress();

    if (c == 0x0202){
      printf("Quitting...\n");
      //clearTScreen();
      break;
    }
    else if(c == 'i'){
      E.mode = 1;
    }else if(c == 27){
      E.mode = 0;
    }else if(E.mode == 1){
      editorInsertChar(&E, c);
    }
    else{
      switch(c){
      case ARROW_UP:
	if(E.cursorY > 0){
	  E.cursorY--;
	  int lineLen = (E.cursorY < E.lineCount) ? E.lines[E.cursorY].size : 0;
	  if (E.cursorX > lineLen) E.cursorX = lineLen;
	}
	break;
      case ARROW_DOWN:
	if(E.cursorY < E.lineCount - 1){
	  E.cursorY++;
	  int lineLen = (E.cursorY < E.lineCount) ? E.lines[E.cursorY].size : 0;
	  if (E.cursorX > lineLen) E.cursorX = lineLen;
	}
	break;
      case ARROW_LEFT:
	if(E.cursorX > 0) E.cursorX--;
	break;
      case ARROW_RIGHT:
	if(E.cursorX < E.lines[E.cursorY].size)
	  E.cursorX++;
	break;
      case 'x':
	editorDeleteChar(&E);
	break;
      }
    }
    editorScroll(&E);
  }
  editorFree(&E);
  return 0;
}

//This is for to transist state Mode back to system.
static DWORD originalMode;

void disableTRawMode(){
  //Restore the original mode saved earlier
  SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), originalMode);
}

void enableTRawMode(){
  //step 1: Get console handle
  HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

  //Step 2: Save original mode (crucial for restoration)
  GetConsoleMode(hIn, &originalMode);

  //Step 3: Modify mode flags
  DWORD raw = originalMode;
  //Step 4: Add flags and Remove flags
  raw &= ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);

  //Step 5: Apply new mode
  SetConsoleMode(hIn, raw);
  //Step 6: REGISTER cleanup 
  atexit(disableTRawMode);
}

// Enable ANSI/VT sequences
void enableANSI(){
    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode;
    GetConsoleMode(hStdout, &mode);
    SetConsoleMode(hStdout, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}

int readKeyPress(){
  INPUT_RECORD record;
  DWORD count;

  while(1){
    /*Bool return type RDI : 1. console input handle,
      2. Buffer to store input events, 3. Number of records to read,
      4. Actual records read
     */
    ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &record, 1, &count);

    if(record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown){
      DWORD vk = record.Event.KeyEvent.wVirtualKeyCode;
      char c = record.Event.KeyEvent.uChar.AsciiChar;
      DWORD ctrlState = record.Event.KeyEvent.dwControlKeyState;

      if(ctrlState & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)){
	if(vk == 'Q' || vk == 'q'){
	  return CTRL_Q;
	}
	if(c == 17){
	  return CTRL_Q;
	}
      }

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

void clearTScreen(){
  printf("\x1b[2J");
  printf("\x1b[H");
  fflush(stdout);
}

int getScreenSize(int *rows, int *cols){
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if(!GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)){
    return -1;
  }

  *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
  *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

  return 0;
}

void initLine(EditorLine *line){
  line->size = 0;
  line->capacity = 16;
  line->chars = (char*)malloc(line->capacity * sizeof(char));
  line->chars[0] = '\0';
}

void lineAppend(EditorLine *line, char c){
  if(line->size + 1 >= line->capacity){
    line->capacity *= 2;
    line->chars = realloc(line->chars, line->capacity * sizeof(char));
  }
  line->chars[line->size] = c;
  line->size++;
  line->chars[line->size] = '\0';
}

//Insert character at position in a line
void lineInsert(EditorLine *line, int at, char c){
  if(at < 0 || at > line->size) at = line->size;

  if(line->size + 1 >= line->capacity){
    line->capacity *= 2;
    line->chars = realloc(line->chars, line->capacity);
  }
  
  memmove(line->chars + at + 1, line->chars + at, line->size - at);
  line->chars[at] = c;
  line->size++;
  line->chars[line->size] = '\0';
}

//Delete character form position in a line
void lineDelete(EditorLine *line, int at){
  if(at < 0 || at >= line->size) return;

  memmove(line->chars + at, line->chars + at + 1, line->size - at);
  line->size--;
  line->chars[line->size] = '\0';
}

void lineFree(EditorLine *line){
  free(line->chars);
}

void initEditor(EditorState *E){
  E->cursorX = 0;
  E->cursorY = 0;

  E->screenRows = 0;
  E->screenCols = 0;

  E->rowOffset = 0;
  E->colOffset = 0;

  E->mode = 0;

  getScreenSize(&E->screenRows, &E->screenCols);

  E->lineCount = 0;
  E->lineCapacity = 16;
  E->lines = (EditorLine*)malloc(E->lineCapacity * sizeof(EditorLine));

  /* //Initialize first line */
  /* initLine(&E->lines[0]); */
}

void editorInsertLine(EditorState *E, int at, const char *text){
  if(at < 0) at = 0;
  if(at > E->lineCount) at = E->lineCount;
  if(E->lineCount + 1 >= E->lineCapacity){
    E->lineCapacity *= 2;
    E->lines = realloc(E->lines, E->lineCapacity * sizeof(EditorLine));
  }

  //shift lines down
  memmove(&E->lines[at + 1], &E->lines[at],
	  sizeof(EditorLine) * (E->lineCount - at));

  //initialize new line
  initLine(&E->lines[at]);

  //fill line with text
  if(text){
    for(int i = 0; text[i]; i++){
      lineAppend(&E->lines[at], text[i]);
    }
  }
  E->lineCount++;
}

void editorFree(EditorState *E){
  for(int i = 0; i < E->lineCount; i++){
    lineFree(&E->lines[i]);
  }
  free(E->lines);
}

//Convert editor cursor to screen coordinates
void editorCursorToScreen(EditorState *E, int *screenX, int *screenY){
  *screenY = E->cursorY - E->rowOffset;
  *screenX = E->cursorX - E->colOffset;

  //Clip to screen bounds
  if(*screenY < 0) *screenY = 0;
  if(*screenY >= E->screenRows)
    *screenY = E->screenRows - 1;
  
  if(*screenX < 0) *screenX = 0;
  if(*screenX >= E->screenCols)
    *screenX = E->screenCols - 1;
}

//Ensure cursor is visible
void editorScroll(EditorState *E){
  //Vertical scrolling
  if(E->cursorY < E->rowOffset){
    E->rowOffset = E->cursorY;
  }
  int contentRows = E->screenRows - 2;
  if(E->cursorY >= E->rowOffset + contentRows){
    E->rowOffset = E->cursorY - contentRows + 1;
  }

  //Horizontal scrolling
  if(E->cursorX < E->colOffset){
    E->colOffset = E->cursorX;
  }

  int lineLen = E->lines[E->cursorY].size;
  if(E->cursorX >= E->colOffset + E->screenCols){
    E->colOffset = E->cursorX - E->screenCols + 1;
  }
  if(lineLen < E->colOffset){
    E->colOffset = lineLen;
  }
}

//Insert character at cursor
void editorInsertChar(EditorState *E, char c){
  // Skip control characters except newline
  if(c < 32 && c != '\n' && c != '\r' && c != '\t'){
    return;
  }
  
  if(E->cursorY == E->lineCount){
    //Add new line if at end
    editorInsertLine(E, E->cursorY, NULL);
  }

  EditorLine *line = &E->lines[E->cursorY];

  if(c == '\n'){
    //Handle Enter/NewLine
    editorInsertLine(E, E->cursorY + 1, line->chars + E->cursorX);

    //Truncate current line
    line->chars[E->cursorX] = '\0';
    line->size = E->cursorX;
    
    E->cursorY++;
    E->cursorX = 0;
  }
  else{
    lineInsert(line, E->cursorX, c);
    E->cursorX++;
  }
}

//Delete character at cursor
void editorDeleteChar(EditorState *E){
  if(E->cursorY == E->lineCount) return;
  if(E->cursorX == 0 && E->cursorY == 0) return;

  EditorLine *line = &E->lines[E->cursorY];

  if(E->cursorX > 0){
    //Delete within line
    lineDelete(line, E->cursorX - 1);
    E->cursorX--;
  }
  else{
    //Merge with previous line
    EditorLine *prev = &E->lines[E->cursorY - 1];
    int prevLen = prev->size;

    //append current line to previous
    for(int i = 0; i < line->size; i++){
      lineAppend(prev, line->chars[i]);
    }
    //Delete current line
    lineFree(line);
    memmove(&E->lines[E->cursorY], &E->lines[E->cursorY + 1],
	    sizeof(EditorLine) * (E->lineCount - E->cursorY - 1));
    E->lineCount--;
    
    E->cursorY--;
    E->cursorX = prevLen;
  }
}

void editorDrawScreen(EditorState *E){
  clearTScreen();
  
  // Layout:
  // Row 1: Content starts here
  // Row (screenRows-2): Status bar
  // Row (screenRows-1): Message bar
  // Row screenRows: Cursor positioning area (or debug)
  
  int contentStartRow = 1;
  int contentRows = E->screenRows - 3;  // -3 for status + message bars
  int statusBarRow = E->screenRows - 2;
  int messageBarRow = E->screenRows - 1;
  
  // Draw content
  for(int screenY = 0; screenY < contentRows; screenY++){
    int fileRow = screenY + E->rowOffset;
    
    printf("\x1b[%d;1H", screenY + contentStartRow);
    printf("\x1b[K");  // Clear line
    
    if(fileRow >= E->lineCount){
      printf("~");
    } else {
      EditorLine *line = &E->lines[fileRow];
      int len = line->size - E->colOffset;
      if(len < 0) len = 0;
      if(len > E->screenCols) len = E->screenCols;
      
      if(len > 0){
        printf("%.*s", len, line->chars + E->colOffset);
      }
    }
  }
  
  // Status bar (inverse video)
  printf("\x1b[%d;1H", statusBarRow);
  printf("\x1b[K");  // Clear line
  printf("\x1b[7m"); // Inverse video
  
  char status[256];
  snprintf(status, sizeof(status), 
           "Row: %d/%d Col: %d Mode: %s",
           E->cursorY + 1, E->lineCount,
           E->cursorX + 1,
           E->mode == 0 ? "NORMAL" : "INSERT");
  printf("%s", status);
  
  // Fill rest of status bar with spaces
  int statusLen = strlen(status);
  if(statusLen < E->screenCols){
    for(int i = 0; i < E->screenCols - statusLen; i++){
      putchar(' ');
    }
  }
  printf("\x1b[0m"); // Reset attributes
  
  // Message bar
  printf("\x1b[%d;1H", messageBarRow);
  printf("\x1b[K");  // Clear line
  
  const char *message = "Press 'i' for insert, ESC for normal, Ctrl+Q to quit";
  printf("%s", message);
  
  // Fill rest of message bar
  int msgLen = strlen(message);
  if(msgLen < E->screenCols){
    for(int i = 0; i < E->screenCols - msgLen; i++){
      putchar(' ');
    }
  }
  
  // Position cursor in content area
  int screenX = E->cursorX - E->colOffset;
  int screenY = E->cursorY - E->rowOffset;
  
  // Bounds checking
  if(screenX < 0) screenX = 0;
  if(screenX >= E->screenCols) screenX = E->screenCols - 1;
  if(screenY < 0) screenY = 0;
  if(screenY >= contentRows) screenY = contentRows - 1;
  
  // Place cursor
  printf("\x1b[%d;%dH", screenY + contentStartRow, screenX + 1);
  
  fflush(stdout);
}
