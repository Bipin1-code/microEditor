
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileio.h"
#include "editor.h"

void editorAppendRow(const char *s, size_t len){
  E.eRows = realloc(E.eRows, sizeof(editorRow) * (E.numRows + 1));

  int at = E.numRows;
  
  E.eRows[at].size = len;
  E.eRows[at].chars = malloc(len + 1);
  memcpy(E.eRows[at].chars, s, len);
  E.eRows[at].chars[len] = '\0';
  
  E.numRows++;
}

static int isBinaryBlock(unsigned char *buf, size_t n){
  for(size_t i = 0; i < n; i++){
    if(buf[i] == 0) return 1;
    if(buf[i] < 9) return 1;
    if(buf[i] > 126){
      if(!(buf[i] == '\n' || buf[i] == '\r' || buf[i] == '\t')){
	return 1;
      }
    }
  }
  return 0;
}

int editorLoadFile(const char *fileName){
  FILE *f = fopen(fileName, "rb");
  if(!f){
    printf("filed to load %s file", fileName);
    return -1;
  }

  unsigned char peek[4096];
  size_t readBytes = fread(peek, 1, sizeof(peek), f);
  fseek(f, 0, SEEK_SET);

  if(isBinaryBlock(peek, readBytes)){
    fclose(f);
    return -2;
  }

  char *line = NULL;
  size_t cap = 0;
  size_t len = 0;
  int c;

  while((c = fgetc(f)) != EOF){
    if(len + 1 >= cap){
      cap = (cap == 0) ? 128 : cap * 2;
      line = realloc(line, cap);
    }

    if(c == '\n'){
      editorAppendRow(line, len);
      len = 0;
    }else if(c == '\r'){
      continue;
    }else{
      line[len++] = (char)c;
    }
  }

  if(len > 0){
    editorAppendRow(line, len);
  }

  free(line);
  fclose(f);

  return 0;
}
