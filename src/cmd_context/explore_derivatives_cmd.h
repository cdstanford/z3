/*
A small patch on top of Z3 to add a command which
explores the state graph (set of all derivatives)
of a regex.
*/

#pragma once

class cmd_context;

void install_explore_derivatives_cmd(cmd_context & ctx, char const * cmd_name = "explore-derivatives");
