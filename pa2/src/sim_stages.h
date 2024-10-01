/**
 * University of Connecticut
 * CSE 4302 / CSE 5302 / ECE 5402: Computer Architecture
 * Fall 2024
 * 
 * 
 * riscy-uconn: sim_stages.h
 * 
 * DO NOT MODIFY THIS FILE
 * 
 */

#pragma once

#include "sim_core.h"

extern int debug;
extern int pipe_trace;
extern int pipe_trace_mode;

struct State fetch(struct State fetch_out);
struct State decode(struct State fetch_out);
struct State execute(struct State decode_out);
struct State memory_stage(struct State alu_out);
unsigned int writeback(struct State memory_out);

void advance_pc(int step);