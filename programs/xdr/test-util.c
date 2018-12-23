#include "cmd.h"

int main(int argc, char **argv)
{
   return CMD_send_command_line_command(argc, argv, NULL, NULL,
         CMD_print_response, NULL, 3000, "test1");
}
