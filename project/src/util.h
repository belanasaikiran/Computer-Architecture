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

#include <stdio.h>

void rdump_file_columns(FILE* file, unsigned columns);
void rdump();
void rdump_pt();
void mdump();
void mdump_modified();
void bdump();
void cdump();
void inst_dump(const char stage[], const unsigned int inst);
void inst_dump_parallel(const char stage[], const unsigned int inst);
void getInstStr(int op, int f3, int f7, char *buffer);
int getDec(char *bin);
void getBin(int num, char *str, int size);