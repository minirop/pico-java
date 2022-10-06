#ifndef GAMEBUINO_H
#define GAMEBUINO_H

#include "classfile.h"

void add_resource(std::string filename, Format format);
std::string encode_filename(std::string filename);
void build_gamebuino(std::string project_name, std::vector<ClassFile> files);

#endif // GAMEBUINO_H
