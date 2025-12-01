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

  if (argc != 2 && argc != 3) { printf("USAGE:\n\n./bin/analyze stat-file\n./bin/analyze stat-file output-name.csv\n\n"); exit(1); }
  
  fin.open(argv[1]);
	if (fin.is_open()) {
    printf("\nStat,          Value\n--------------------\n\n");
    while (getline(fin, line)) {     
      istringstream iss(line);
      
      while (iss >> subs) {
        // numCycles
        if (prev == "board.processor.start.core.numCycles") { printf("%s,     %s\n", prev.c_str(), subs.c_str()); }
        if (prev == "board.processor.switch.core.numCycles") { printf("%s,     %s\n", prev.c_str(), subs.c_str()); }
        if (prev == "board.processor.switch0.core.numCycles") { printf("%s,     %s\n", prev.c_str(), subs.c_str()); }
        if (prev == "board.processor.switch1.core.numCycles") { printf("%s,     %s\n", prev.c_str(), subs.c_str()); }
        if (prev == "board.processor.switch2.core.numCycles") { printf("%s,     %s\n", prev.c_str(), subs.c_str()); }
        if (prev == "board.processor.switch3.core.numCycles") { printf("%s,     %s\n", prev.c_str(), subs.c_str()); }

        // Instructions
        if (prev == "board.processor.start.core.commitStats0.numInsts") { printf("%s,  %s\n", prev.c_str(), subs.c_str());}
        if (prev == "board.processor.switch.core.commitStats0.numInsts") { printf("%s,  %s\n", prev.c_str(), subs.c_str());}
        if (prev == "board.processor.switch.core.commitStats0.ipc") { printf("%s,  %s\n", prev.c_str(), subs.c_str());}
        if (prev == "board.processor.switch0.core.ipc") { printf("%s,  %s\n", prev.c_str(), subs.c_str());}

        // Accesses
        //l1
        if (prev == "board.cache_hierarchy.l1i-cache-0.overallAccesses::total") { printf("%s,  %s\n", prev.c_str(), subs.c_str());}
        if (prev == "board.cache_hierarchy.l1d-cache-0.overallAccesses::total") { printf("%s,  %s\n", prev.c_str(), subs.c_str());}
        // l2
        if (prev == "board.cache_hierarchy.l2cache.overallAccesses::total") {  printf("%s,  %s\n", prev.c_str(), subs.c_str()); }
        if (prev == "board.cache_hierarchy.l2-cache-0.overallAccesses::total") { printf("%s,  %s\n", prev.c_str(), subs.c_str());}
        // metadata cache
        if (prev == "board.memory.secure_memory.l3.overallAccesses::total") { printf("%s,  %s\n", prev.c_str(), subs.c_str());}
        if (prev == "board.memory.secure_memory.metadata_requests") { printf("%s,  %s\n", prev.c_str(), subs.c_str());}

        // Hits 
        //l1
        if (prev == "board.cache_hierarchy.l1d-cache-0.overallHits::total") { printf("%s,  %s\n", prev.c_str(), subs.c_str());}
        if (prev == "board.cache_hierarchy.l1i-cache-0.overallHits::total") { printf("%s,  %s\n", prev.c_str(), subs.c_str());}
        // l2
        if (prev == "board.cache_hierarchy.l2-cache-0.overallHits::total") {  printf("%s,    %s\n", prev.c_str(), subs.c_str()); }
        if (prev == "board.cache_hierarchy.l2cache.overallHits::total") {  printf("%s,    %s\n", prev.c_str(), subs.c_str()); }
        // metadata cache
        if (prev == "board.memory.secure_memory.l3.overallHits::total") { printf("%s,  %s\n", prev.c_str(), subs.c_str());}


        // Misses
        //l1
        if (prev == "board.cache_hierarchy.l1d-cache-0.overallMissRate::total") { printf("%s,  %s\n", prev.c_str(), subs.c_str());}
        if (prev == "board.cache_hierarchy.l1i-cache-0.overallMissRate::total") { printf("%s,  %s\n", prev.c_str(), subs.c_str());}
        // l2
        if (prev == "board.cache_hierarchy.l2cache.overallMisses::total") {  printf("%s,    %s\n", prev.c_str(), subs.c_str()); }
        if (prev == "board.cache_hierarchy.l2-cache-0.overallMissRate::total") { printf("%s,  %s\n", prev.c_str(), subs.c_str());}
        // metadata cache
        if (prev == "board.memory.secure_memory.l3.overallMissRate::total") { printf("%s,  %s\n", prev.c_str(), subs.c_str());}
        if (prev == "board.memory.secure_memory.metadata_misses") { printf("%s,  %s\n", prev.c_str(), subs.c_str());}

        prev = subs;
      }
    }
  }

  fin.close();
  return 1;
}
