#ifndef LINEANALYSER_H
#define LINEANALYSER_H

#include "globals.h"

std::vector<Instruction> lineAnalyser(Buffer & buffer, const std::string & name, std::vector<u2> lineNumbers);

#endif // LINEANALYSER_H
