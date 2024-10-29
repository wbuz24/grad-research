// Will Buziak
// Process stats file from command line and output jgraph
#include <iostream>
#include <fstream>
#include <cstdio>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
using namespace std;

int main(int argc, char** argv)
{
  string fname, line, s, title;
  ifstream file;
  ofstream ofile;
  istringstream ss;
  ostringstream os;
  vector <string> sv;
  int i, buf;
  double simt;
  long long mem;

  if (argc != 2 && argc != 3) { fprintf(stderr, "USAGE:\n./processGraph /path/To/stats.txt <Graph Title>\n"); exit(1); }

  // grab filename & graph title
  fname = argv[1];
  if (argc == 3) title = argv[2];

  printf("Processing %s\n", fname.c_str());

  // open file
  file.open(fname);
  if (!file) { fprintf(stderr, "%s failed to open\n", fname.c_str()); exit(1); }
  
  // Process each line
  while (getline(file, line)) {
    sv.clear(); // clear vector/streams and insert new line
    ss.clear();
    ss.str(line);

    while (ss >> s) sv.push_back(s); // process string stream into vector

    // print each line
    for (i = 0; i < sv.size(); i++) { 
      if (sv[i] == "simSeconds") {
        sscanf(sv[i+1].c_str(), "%lf", &simt); 
        printf("Sim time: %.2lf (milliseconds)\n", simt * 1000); 
      }
      if (sv[i] == "hostSeconds") printf("Host time: %s (seconds)\n", sv[i+1].c_str()); 
      if (sv[i] == "hostMemory") {
        sscanf(sv[i+1].c_str(), "%lld", &mem);
        printf("Host memory : %lld (kB)\n", mem / 1000); 
      }

      if (sv[i] == "system.cpu.numCycles") printf("Simulated CPU cycles: %s\n", sv[i+1].c_str()); 
    }
  }

  // close input file stream
  file.close();

  // Build the jgr output file
  if (argc == 3) {
    os << title << ".jgr";
    s = os.str();
  }
  else s = "stats.jgr";

  // open the output file steam
  ofile.open(s);
  if (!ofile) { fprintf(stderr, "output file %s failed to open\n", s.c_str()); exit(1); }

  // Create a new graph
  ofile << "newgraph\n\n";

  // Create and define the xaxis parameters
  ofile << "xaxis size 2\n  hash_labels fontsize 20\n";
  ofile << "  hash_label at 1 : hello world\n\n";
  ofile << "  hash_label at 2 : Sam's Homework\n\n"; // create labels instead of x axis
  // define yaxis and create gray horizontal lines
  ofile << "yaxis size 2 min 0 max 100\n  grid_lines grid_gray .7\n\n"; 

  ofile << "legend top\n\n";

  return 0;
}
