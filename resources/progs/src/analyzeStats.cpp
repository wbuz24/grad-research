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
  ofstream ofile;
  string line, subs, prev;
  string misses, instructions, accesses, numCycles;

  if (argc != 2 || argc != 3) { printf("USAGE:\n\n./bin/analyze stat-file\n./bin/analyze stat-file output-name.csv\n\n"); exit(1); }
  
  fin.open(argv[1]);
	if (fin.is_open()) {
    while (getline(fin, line)) {     
      istringstream iss(line);
      
      while (iss >> subs) {
        if (prev == "board.processor.start.core.numCycles") { numCycles = subs; }
        if (prev == "board.processor.start.core.commitStats0.numInsts") { instructions = subs; }
        if (prev == "board.cache_hierarchy.l2cache.overallMisses::total") { misses = subs; }
        if (prev == "board.cache_hierarchy.l2cache.overallAccesses::total") { accesses = subs;}
        prev = subs;
      }
    }
  }

  fin.close();

  printf("\nStat,          Value\n--------------------\n\n");
  printf("numCycles,     %s\n", numCycles.c_str());
  printf("Commmit Inst,  %s\n", instructions.c_str());
  printf("LLC Accesses,  %s\n", accesses.c_str());
  printf("LLC Misses,    %s\n", misses.c_str());

  if (argc == 3) {
    ofile.open(argv[2]);
    printf("\n\n\nCreating results.csv\n\n");
    
    ofile << "Stat,        Value\n\n";
    ofile << "numCycles,    " << numCycles << "\n";
    ofile << "Commit Inst,  " << instructions << "\n";
    ofile << "LLC Accesses, " << accesses << "\n";
    ofile << "LLC Misses,   " << misses << "\n";

    ofile.close();
  }
  return 1;
}
