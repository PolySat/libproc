#include <polysat3/cmd.h>
#include <polysat3/xdr.h>
#include "test_schema.h"

int main(int argc, char **argv)
{
    enum XDR_PRINT_STYLE pstyle = XDR_PRINT_HUMAN;
    IPC_TEST_forcelink();
   return CMD_send_command_line_command_blocking(argc, argv, NULL, 
         CMD_print_response,&pstyle, 3000, "test1", &pstyle);
}
