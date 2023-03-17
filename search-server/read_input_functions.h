#pragma once

#include <string>
#include <vector>
#include <iostream>

#include "paginator.h"

std::string ReadLine();
int ReadLineWithNumber();

// Originaly was in paginator files, but this one requires a .cpp to work, and there are no paginator.cpp in task files
std::ostream& operator<< (std::ostream& out, const Paginator::Page& page);
