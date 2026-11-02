#ifndef CLI_H
#define CLI_H

void cli_init(void);
void cli_print_logo(void);
void cli_process_command(const char* cmd);
void cli_run(void);

#endif