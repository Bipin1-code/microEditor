
#ifndef EDITORBUF_H
#define EDITORBUF_H

typedef struct{
  char *b;
  int len;
} eBuf;

#define EDITORBUF_INIT {NULL, 0}

void ebufAppend(eBuf *eb, const char *s, int len);
void ebufFree(eBuf *eb);

#endif
