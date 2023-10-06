#ifndef COMMANDS_H
#define COMMANDS_H

const size_t MAX_CMD = 100;

struct usr_cmd {

    char * name;
    int    code;

};

// command cmd_hlt = {
    // .cmd_name = "hlt",
    // .cmd_code = -1
// };
//
// command cmd_push = {
    // .cmd_name = "push",
    // .cmd_code = 1
// };
//
// command cmd_pop = {
    // .cmd_name = "pop",
    // .cmd_code = 2
// };
//
// command cmd_in = {
    // .cmd_name = "in",
    // .cmd_code = 3
// };
//
// command cmd_out = {
    // .cmd_name = "out",
    // .cmd_code = 4
// };
//
// command cmd_add = {
    // .cmd_name = "add",
    // .cmd_code = 11
// };
//
// command cmd_sub = {
    // .cmd_name = "sub",
    // .cmd_code = 12
// };
//
// command cmd_mul = {
    // .cmd_name = "mul",
    // .cmd_code = 13
// };
//
// command cmd_div = {
    // .cmd_name = "div",
    // .cmd_code = 14
// };

// command cmd_list[] = {cmd_hlt, cmd_push, cmd_pop, cmd_in, cmd_out, cmd_add, cmd_sub, cmd_mul, cmd_div};

#endif // COMMANDS_H
