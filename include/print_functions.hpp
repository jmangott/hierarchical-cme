#ifndef PRINT_FUNCTIONS_HPP
#define PRINT_FUNCTIONS_HPP

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

#include <generic/matrix.hpp>
#include <generic/storage.hpp>

#ifdef __OPENMP__
#include <omp.h>
#endif

#include "grid_class.hpp"

// Print progress bar
void PrintProgressBar(Index ts, Index kNsteps, std::chrono::_V2::system_clock::time_point start_time, double norm);

// Print diagnostic information
void PrintDiagnostics(grid_info grid, double min_prop, double max_prop, double tau, bool second_order, Index n_substeps);

#endif