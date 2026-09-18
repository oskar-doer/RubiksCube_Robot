#pragma once
#include <cstdint>
#include <string>
#include "cube_model.h"
bool prepareSolver();
std::string solveCube(const cube_t& cube, uint8_t& moves, uint32_t searchTimeMs = 3000);
bool verifySolution(const cube_t& cube, const std::string& solution, int& count);
