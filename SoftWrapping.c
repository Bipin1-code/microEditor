
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


typedef struct{
  int width;
  int height;
  char **lines;
} VirtualScreen;

VirtualScreen *createVirtualScreen(int width, int height){
  VirtualScreen *vs = malloc(sizeof(VirtualScreen));
  vs->width = width;
  vs->height = height;

  vs->lines = malloc(height * sizeof(char*));
  for(int i = 0; i < height; i++){
    vs->lines[i] = calloc(width + 1, sizeof(char));
    memset(vs->lines[i], ' ', width);
    vs->lines[i][width] = '\0';
  }
  return vs;
}

void freeVirtualScreen(VirtualScreen *vs){
  for(int i = 0; i < vs->height; i++){
    free(vs->lines[i]);
  }
  free(vs->lines);
  free(vs);
}

void vsWriteLine(VirtualScreen *vs, int row, int col, const char *text){
  if(row < 0 || row >= vs->height) return;
  int len = strlen(text);
  if(col >= vs->width) return;

  int maxCopy = vs->width - col;
  if(len > maxCopy) len = maxCopy;
  memcpy(&vs->lines[row][col], text, len);
}

int vsWriteWrappedLine(VirtualScreen *vs, int startRow, int colStart, const char *line) {
    int row = startRow;
    int col = colStart;
    int width = vs->width;

    const char *p = line;

    while (*p) {
      // Skip leading spaces (but if at start of line, don't skip, print space)
      while (*p && isspace((unsigned char)*p)) {
	p++;
	
	if(col == 0)
	  continue;

	if(col < width){
	  vs->lines[row][col++] = ' ';
	}else{
	  row++;
	  col = 0;
	}
	while(*p && isspace((unsigned char)*p))
	  p++;
      }

      if (!*p) break;

      // Extract next word (non-space chars)
      const char *wordStart = p;
      int wordLen = 0;
      while(*p && !isspace((unsigned char)*p)){
	p++;
	wordLen++;
      }

      // If word longer than width, break the word itself
      if(wordLen > width){
	int i = 0;
	while(i < wordLen){
	  if(row >= vs->height)
	    return row - startRow; // no more space

	  int spaceLeft = width - col;
	  int toCopy = (wordLen - i < spaceLeft) ? wordLen - i : spaceLeft;

	  memcpy(&vs->lines[row][col], wordStart + i, toCopy);
	  col += toCopy;
	  i += toCopy;

	  if(col >= width){
	    row++;
	    col = 0;
	  }
	}
      }else{
	// If word doesn't fit on current line, move to next line
	if(col + wordLen > width){
	  row++;
	  col = 0;
	  if(row >= vs->height)
	    return row - startRow; // no more space
	}
	memcpy(&vs->lines[row][col], wordStart, wordLen);
	col += wordLen;
      }
    }
    return row - startRow + 1; // total rows used
}


void vsRender(VirtualScreen *vs){
  printf("\x1b[2J");
  printf("\x1b[H");
  for(int i = 0; i < vs->height; i++){
    printf("\x1b[47;30m");
    printf("%s", vs->lines[i]);
    printf("\x1b[0m");
    printf("\r\n");
  }
  fflush(stdout);
}

 // return number of chars read
int my_getline(char **lineptr, size_t *n, FILE *stream){
  if(!lineptr || !n || !stream)
    return -1;

  size_t pos = 0;
  int c;

  if(*lineptr == NULL || *n == 0){
    *n = 128;
    *lineptr = malloc(*n);
    
    if(!*lineptr)
      return -1;
  }

  while((c = fgetc(stream)) != EOF){
    if (pos + 1 >= *n) {
      size_t new_size = *n * 2;
      char *new_ptr = realloc(*lineptr, new_size);
      
      if(!new_ptr)
	return -1;
      
      *lineptr = new_ptr;
      *n = new_size;
    }

    (*lineptr)[pos++] = (char)c;
    if(c == '\n') break;
  }

  if(pos == 0 && c == EOF) return -1;

  (*lineptr)[pos] = '\0';
  return (int)pos; 
}


int main(){
  VirtualScreen *vs = createVirtualScreen(100, 30);

  const char *fileName = "testFile.txt";
  FILE *f = fopen(fileName, "r");
  if(!f){
    fprintf(stderr, "Failed to open %s file", fileName);
    return 1;
  }
  
  char *line = NULL;
  size_t cap = 0;
  ssize_t read;
  int currentRow = 0;

  while((read = my_getline(&line, &cap, f)) != -1){
    if(currentRow >= vs->height) break;  // no more space

    // Remove trailing newline
    if(read > 0 && (line[read - 1] == '\n' || line[read - 1] == '\r')){
      line[read - 1] = '\0';
      read--;
    }

    int usedRows = vsWriteWrappedLine(vs, currentRow, 0, line);
    currentRow += usedRows;
  }

  free(line);
  fclose(f);

  vsRender(vs);
  
  freeVirtualScreen(vs);
  
  return 0;
}
