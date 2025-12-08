#pragma once

#include <string>

struct ClientOptions
{
    bool verbose    = false;
    bool useDefault = false;
};

ClientOptions parseOptions(int argc, char* argv[]);
void configureLogger(bool verbose);
