#ifndef GAMEBUINO_H
#define GAMEBUINO_H

#include "globals.h"

enum class Format
{
    Rgb565,
    Indexed,
};

void add_resource(std::string filename, Format format);
std::string encode_filename(std::string filename);
void build_gamebuino(std::string project_name);

#endif // GAMEBUINO_H
