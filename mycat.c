
#include <windows.h>
#include <stdio.h>

void copyHandle(HANDLE in, HANDLE out){
  char buf[4096];
  DWORD bytesRead, bytesWritten;
  
  while(ReadFile(in, buf, sizeof(buf),  &bytesRead, NULL)) {
    if(bytesRead <= 0) return;
    DWORD off = 0;
    while(off < bytesRead){
      if(!WriteFile(out, buf + off, bytesRead - off, &bytesWritten, NULL)){
	fprintf(stderr, "WriteFile failed\n");
	return;
      }
      off += bytesWritten;
    }
  }
}

int main(int argc, char *argv[]){
 
  HANDLE hout = GetStdHandle(STD_OUTPUT_HANDLE);

  if(argc == 1){
    HANDLE hin = GetStdHandle(STD_INPUT_HANDLE);
    copyHandle(hin, hout);
    return 0;
  }

  for(int i = 1; i < argc; i++){
    HANDLE hFile = CreateFileA(argv[i],
			       GENERIC_READ,
			       FILE_SHARE_READ,
			       NULL,
			       OPEN_EXISTING,
			       FILE_ATTRIBUTE_NORMAL,
			       NULL
			       );

    if(hFile == INVALID_HANDLE_VALUE){
      fprintf(stderr, "kat: cannot open %s\n", argv[i]);
      continue;
    }
    copyHandle(hFile, hout);
    CloseHandle(hFile);
  }
 
  return 0; 
}
