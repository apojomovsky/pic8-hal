/* Cross-compile sizecheck for pic8-console. */

#include "pic8_console.h"

static void stub(uint8_t argc, char **argv, void *ctx)
{
    (void)argc;
    (void)argv;
    (void)ctx;
}

int main(void)
{
    static const pic8_console_cmd_t table[] = {
        { "ping", stub, "ping" },
    };
    pic8_console_t con;
    PIC8_CONSOLE_INIT(&con, table, 0);
    pic8_console_print_help(&con);
    return 0;
}
