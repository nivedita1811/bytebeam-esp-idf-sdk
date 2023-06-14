//#include "user_actions.h"

int handle_ota(char *args, char *action_id);

struct action_functions_map { const char *name; int (*func)(char *args, char *action_id); };

extern struct action_functions_map action_funcs[];

int handle_actions(char* action_received);