/**
 * University of Connecticut
 * CSE 4302 / CSE 5302 / ECE 5402: Computer Architecture
 * Fall 2024
 * 
 * Project: MultiCycle Stall + OOO + Seperate Execution Units + BP + Cache
 * 
 * riscy-uconn: instruction_map.h
 * 
 * DO NOT MODIFY THIS FILE
 * 
 */

#pragma once

#include "sim_core.h"

extern int debug;
extern int pipe_trace;
extern int pipe_trace_mode;

struct State fetch();
struct State decode();
struct State execute();
struct State execute_ld_st();
struct State execute_2nd_ld_st();
void writeback();

/* Related to branch prediction */
unsigned int BTB_lookup(unsigned int inst_addr);
unsigned int BTB_target(unsigned int inst_addr);
unsigned int predict_direction(unsigned int inst_addr);
void BTB_update(unsigned int inst_addr, unsigned int branch_target);
void direction_update(unsigned int direction, unsigned int inst_addr);

/* Related to data caching */
unsigned int dcache_lookup_4_way(unsigned int addr_mem);
void dcache_update_4_way(unsigned int addr_mem, int line);

unsigned int dcache_lookup_2_way(unsigned int addr_mem);
void dcache_update_2_way(unsigned int addr_mem, int line);

unsigned int dcache_lookup_DM(unsigned int addr_mem);
void dcache_update_DM(unsigned int addr_mem);


/* Related to simulator loop */
void advance_pc(int step);