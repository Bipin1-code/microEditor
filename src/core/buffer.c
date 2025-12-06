
#include "buffer.h"
#include <stdlib.h>
#include <string.h>

void ebufAppend(eBuf *eb, const char *s, int len){

  char *newBuf = realloc(eb->b, eb->len + len);

  if(newBuf == NULL) return;

  memcpy(&newBuf[eb->len], s, len);

  eb->b = newBuf;
  eb->len += len;
}

void ebufFree(eBuf *eb){
  free(eb->b);
  eb->b = NULL;
  eb->len = 0;
}
