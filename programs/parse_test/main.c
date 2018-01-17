#include <proclib.h>
#include <ipc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>


static int next_param(char *str, char **start, char **end)
{
   int inQuote = 0;

   // End of string edge cases
   if (!str) {
      *start = NULL;
      *end = NULL;
      return 0;
   }
   if (!*str) {
      *start = str;
      *end = str;
      return 0;
   }

   // Skip leading whitespace
   *start = str;
   while( isspace(**start) )
      (*start)++;
   if (!**start) {
      *end = *start;
      return 0;
   }
   if (**start == '\'')
      inQuote = 1;
   if (**start == '"')
      inQuote = 2;

   for( *end = *start + 1; *end; (*end)++) {
      if (!**end || (isspace(**end) && !inQuote)) {
         (*end)--;
         return 1;
      }

      if (**end == '\'') {
         if (!inQuote)
            inQuote = 1;
         else if (inQuote == 1)
            inQuote = 0;
      }
      else if (**end == '"') {
         if (!inQuote)
            inQuote = 2;
         else if (inQuote == 2)
            inQuote = 0;
      }
   }

   fprintf(stderr, "Should never get here!");
   return 1;
}

static char **parse_cmd(char *cmd)
{
   char *start = cmd, *end;
   int cnt = 0;
   char **argv = NULL;
   int len;

   while(next_param(start, &start, &end)) {
      cnt++;
      start = end + 1;
      if (*start)
         start++;
   }

   printf("%d args\n", cnt);

   argv = malloc(sizeof(char*) * (cnt + 1) );
   if (!argv) {
      fprintf(stderr, "Failed to allocate memory\n");
      return NULL;
   }

   cnt = 0;
   start = cmd;
   while(next_param(start, &start, &end)) {
      argv[cnt] = start;
      start = end + 1;
      if (*start) {
         *start = 0;
         start++;
      }

      if (argv[cnt][0] == '"' || argv[cnt][0] == '\'') {
         len = strlen(argv[cnt]) - 1;
         if (argv[cnt][0] == argv[cnt][len]) {
            argv[cnt][len] = 0;
            argv[cnt]++;
         }
      }

      cnt++;
   }

   return argv;
}

// Entry point
int main(int argc, char *realargv[])
{
   char cmd[1024*4];
   char **argv;
   int i;

   while (gets(cmd)) {
      argv = parse_cmd(cmd);
      if (argv) {
         for(i = 0; argv[i]; i++) {
            printf("\t***%s***\n", argv[i]);
         }
      }
   }

  return 0;
}
