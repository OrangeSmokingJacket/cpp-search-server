#pragma once

#include <string>
#include <vector>
#include <iostream>

#include "string_processing.h"
#include "document.h"
#include "paginator.h"

std::string ReadLine();
int ReadLineWithNumber();
std::vector<int> ReadLineOfNumbers();

void PrintDocument(const Document& document);
std::ostream& operator<< (std::ostream& out, const Document& document);
std::ostream& operator<< (std::ostream& out, const Paginator::Page& page);