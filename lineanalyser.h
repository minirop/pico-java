#ifndef LINEANALYSER_H
#define LINEANALYSER_H

#include "globals.h"

std::vector<Instruction> lineAnalyser(Buffer & buffer, const std::string & name, std::vector<std::tuple<u2, u2>> lineNumbers);

#endif // LINEANALYSER_H
