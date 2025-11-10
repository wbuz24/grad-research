
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <map>
using namespace std;

int main(int argc, char** argv) {
  ifstream fin;
  string line, subs, prev;
  string misses, instructions, accesses;
  uint32_t miss, inst, acc, missRate, accperInst;

  if (argc != 2) { printf("./analyzeStats stat-file\n"); exit(1); }
  
  fin.open(argv[1]);
	if (fin.is_open()) {
    while (getline(fin, line)) {     
      istringstream iss(line);
      
      while (iss >> subs) {
        if (prev == "simInsts") { instructions = subs; }
        if (prev == "board.cache_hierarchy.l2cache.overallMisses::total") { misses = subs; }
        if (prev == "board.cache_hierarchy.l2cache.overallAccesses::total") { accesses = subs; }
        prev = subs;
      }
    }
  }

  miss = atoi(misses.c_str());
  inst = atoi(instructions.c_str());
  acc = atoi(accesses.c_str());

  missRate = miss / acc;
  accperInst = acc / inst;

  printf("1000 * %u * %u = %u\n", missRate, accperInst, 1000 * missRate * accperInst);

  return 1;
}
