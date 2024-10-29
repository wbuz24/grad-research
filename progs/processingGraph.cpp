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
  int i;
  double simt, mem, hostt;

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
        simt = simt * 1000;
        printf("Sim time: %.2lf (milliseconds)\n", simt); 
      }
      if (sv[i] == "hostSeconds") {
        sscanf(sv[i+1].c_str(), "%lf", &hostt);
        printf("Host time: %lf (seconds)\n", hostt); 
      }
      if (sv[i] == "hostMemory") {
        sscanf(sv[i+1].c_str(), "%lf", &mem);
        mem = mem / 1000000;
        printf("Host memory : %lf (MB)\n", mem); 
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
  ofile << "xaxis size 2 min .8 max 3.3\n  hash 1 mhash 0 shash 0\n  no_auto_hash_labels\n";
  ofile << "  hash_labels fontsize 12 font Times-Italix hjl rotate -60\n";
  ofile << "  hash_label at 1 : Sim Time (milliseconds)\n\n";
  ofile << "  hash_label at 2 : Host Time (seconds)\n\n";
  ofile << "  hash_label at 3 : Host memory (MB)\n\n"; // create labels instead of x axis

  // define yaxis and create gray horizontal lines
  ofile << "yaxis size 2 min 0 max 1\n  grid_lines grid_gray .7\n\n"; 

  // When using the gray lines you have to redraw the x axis
  ofile << "newline pts 0.8 0 3.2 0\n\n";

  // Yellow bars
  ofile << "newcurve marktype xbar cfill 0 .9 .6\n\n";

  ofile << "marksize .1 .08\n\n";

  if (argc == 3) ofile << "label : " << title << "\n";
  else ofile << "label : stats.txt\n";

  ofile << "pts\n" << "  1 " << simt << " 2 " << hostt << " 3 " << mem;

  return 0;
}
