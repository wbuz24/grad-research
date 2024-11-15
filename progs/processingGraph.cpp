// Will Buziak
// Process stats file from command line and output jgraph

// jgraph -P input.jgr | ps2pdf - | magick -density 300 - -quality 100 out.jpg

#include <iostream>
#include <fstream>
#include <cstdio>
#include <vector>
#include <string>
#include <sstream>
#include <cstring>
using namespace std;

void barGraph(vector <double> stats, string fname, ofstream& ofile)
{
  double simt, hostt, mem;

  simt = stats[0];
  hostt = stats[1];
  mem = stats[2];

  // Create another new graph
  ofile << "newgraph\n\n";

  ofile << "x_translate 4\n\n";

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
  ofile << "newcurve marktype xbar cfill 0 1 .6\n\n";

  ofile << "marksize .1 .08\n\n";

  ofile << "label : " << fname << "\n";

  ofile << "pts\n" << "  1 " << simt << " 2 " << mem;

  ofile << "\n\n";
}

vector <double> extractData(string filename) 
{
  int i;
  double simt, mem, hostt;
  ifstream file;
  istringstream ss;
  string line, s;
  vector <string> sv;
  vector <double> res;

  // open file
  file.open(filename);
  if (!file) { fprintf(stderr, "%s failed to open\n", filename.c_str()); exit(1); }

  printf("\nProcessing %s\n\n", filename.c_str());

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
  fflush(stdout);
  file.close();
  res.push_back(simt);
  res.push_back(hostt);
  res.push_back(mem);

  return res;
}

int main(int argc, char** argv)
{
  string fname, f2name, str, title;
  ofstream ofile;
  ostringstream os;
  double simt, mem, hostt;
  vector <double> stats;

  if (argc != 2 && argc != 3 && argc != 4) { fprintf(stderr, "USAGE:\n./processGraph /path/To/stats.txt <Graph Title>\n"); exit(1); }

  // grab filename & graph title
  fname = argv[1];

  stats = extractData(fname);

  simt = stats[0];
  hostt = stats[1];
  mem = stats[2];

  // Build the jgr output file
  if (argc == 4) {
    os << argv[3];
    str = os.str();
  }
  else if (argc == 3) str = "Compare.jgr";
  else str = "stats.jgr";

  // open the output file steam
  ofile.open(str);
  if (!ofile.is_open()) { fprintf(stderr, "%s has failed to open\n", str.c_str()); exit(1); }

  barGraph(stats, f2name, ofile);

  // If there are two graphs supplied, put them on the same page

  if (argc == 4) {
    stats.clear();

    f2name = argv[2];
    stats = extractData(f2name);

    barGraph(stats, f2name, ofile);
  }

  ofile.close();

  printf("\nWriting to: %s\n", str.c_str());
  return 0;
}
