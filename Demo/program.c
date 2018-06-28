#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
  if(argc < 2)
  {
   printf("Too few arguments");
    return 0;
  }
  char* te = argv[1];
  const char* one = "sudo babeltrace run --component=A:source.text.dmesg --params='no-extract-timestamp=no' --key=path --value ~/Desktop/babeltrace_exp2/";
  const char* two = " --component=B:sink.ctf.fs --key=path --value ~/Desktop/babeltrace_exp2";
  const char* three = " --connect=A:B";
  FILE* fp;
  char* final;
  char* output;
  final = malloc(strlen(one)+strlen(two)+strlen(three)+strlen(te)+1);
  strcpy(final, one); 
  strcat(final, te);
  strcat(final, two);
  strcat(final, three);
  strcat(final, "\0");
  printf("%s\n", final);
  fp = system (final);
  if(fp == -1)
  {
     printf("System() failed to execute");
     return 0;
  } 
 return 0;
}
