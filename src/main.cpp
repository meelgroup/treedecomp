// Standalone tool: read a DIMACS CNF, build the primal graph, run FlowCutter,
// and print per-variable tree-decomposition scores. Mirrors the cutoffs and
// options used by ganak's counter.cpp so results can be compared directly.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "argparse.hpp"
#include "graph.hpp"
#include "IFlowCutter.hpp"
#include "TreeDecomposition.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::string;
using std::vector;

struct Config {
  // FlowCutter
  int64_t td_steps = 100000;
  int td_iters = 900;

  // Graph cutoffs (mirror ganak conf.*)
  int td_max_edges = 70000;
  double td_max_density = 0.3;
  int td_max_edge_var_ratio = 30;
  int td_varlim = 150000;
  int do_td_contract = 1;

  int verb = 1;
  string input;
};

static vector<vector<int>> read_cnf(const string& fname, int& nvars) {
  std::istream* in = nullptr;
  std::ifstream fin;
  if (fname == "-" || fname.empty()) {
    in = &std::cin;
  } else {
    fin.open(fname);
    if (!fin) { cerr << "ERROR: cannot open '" << fname << "'" << endl; exit(1); }
    in = &fin;
  }

  nvars = 0;
  int ncls = 0;
  vector<vector<int>> cls;
  string line;
  bool got_header = false;
  while (std::getline(*in, line)) {
    if (line.empty() || line[0] == 'c') continue;
    if (line[0] == 'p') {
      std::istringstream iss(line);
      string p, cnf;
      iss >> p >> cnf >> nvars >> ncls;
      got_header = true;
      continue;
    }
    std::istringstream iss(line);
    vector<int> c;
    int lit;
    while (iss >> lit) {
      if (lit == 0) break;
      c.push_back(lit);
    }
    if (!c.empty()) cls.push_back(std::move(c));
  }
  if (!got_header) { cerr << "ERROR: missing 'p cnf' header" << endl; exit(1); }
  return cls;
}

static TWD::Graph build_primal(int nvars, const vector<vector<int>>& cls) {
  TWD::Graph g(nvars);
  for (const auto& c : cls) {
    for (size_t i = 0; i < c.size(); i++) {
      int vi = std::abs(c[i]) - 1;
      for (size_t j = i + 1; j < c.size(); j++) {
        int vj = std::abs(c[j]) - 1;
        if (vi != vj) g.addEdge(vi, vj);
      }
    }
  }
  return g;
}

int main(int argc, char** argv) {
  Config conf;
  argparse::ArgumentParser program("treedecomp", "1.0.0",
      argparse::default_arguments::help);

  auto fc_int    = [](const string& s) { return std::stoi(s); };
  auto fc_int64  = [](const string& s) { return (int64_t)std::stoll(s); };
  auto fc_double = [](const string& s) { return std::stod(s); };
  auto add_int = [&](const char* name, int& var, const char* h) {
    program.add_argument(name)
        .action([&var, fc_int](const auto& a){ var = fc_int(a); })
        .default_value(var).help(h);
  };
  auto add_i64 = [&](const char* name, int64_t& var, const char* h) {
    program.add_argument(name)
        .action([&var, fc_int64](const auto& a){ var = fc_int64(a); })
        .default_value(var).help(h);
  };
  auto add_dbl = [&](const char* name, double& var, const char* h) {
    program.add_argument(name)
        .action([&var, fc_double](const auto& a){ var = fc_double(a); })
        .default_value(var).help(h);
  };

  add_i64("--tdsteps", conf.td_steps,  "FlowCutter max steps");
  add_int("--tditers", conf.td_iters,  "FlowCutter iterations (restarts)");
  add_int("--tdmaxedges", conf.td_max_edges, "Skip TD if primal has more than this many edges");
  add_dbl("--tdmaxdensity", conf.td_max_density, "Skip TD if primal density exceeds this");
  add_int("--tdmaxedgeratio", conf.td_max_edge_var_ratio, "Skip TD if edge/var ratio exceeds this");
  add_int("--tdvarlim", conf.td_varlim, "Skip TD if nvars exceeds this");
  add_int("--tdcontract", conf.do_td_contract, "Contract high-numbered vars before running TD");
  add_int("-v", conf.verb, "Verbosity");
  program.add_argument("input")
      .action([&](const auto& a){ conf.input = a; })
      .default_value(string("-"))
      .help("DIMACS CNF file (default: stdin)");

  try { program.parse_args(argc, argv); }
  catch (const std::exception& e) {
    cerr << e.what() << endl << program; return 1;
  }

  int nvars = 0;
  auto cls = read_cnf(conf.input, nvars);
  if (conf.verb >= 1)
    cout << "c parsed nvars=" << nvars << " clauses=" << cls.size() << endl;

  if (nvars <= 0) { cerr << "ERROR: no variables" << endl; return 1; }
  if (nvars > conf.td_varlim) {
    cerr << "c too many vars (" << nvars << " > " << conf.td_varlim
         << "), skipping TD" << endl;
    return 0;
  }

  auto primal = build_primal(nvars, cls);
  const uint64_t n2 = (uint64_t)nvars * (uint64_t)nvars;
  const double density = n2 ? (double)primal.numEdges() / (double)n2 : 0.0;
  const double edge_var_ratio = nvars ? (double)primal.numEdges() / (double)nvars : 0.0;
  if (conf.verb >= 1) {
    cout << "c primal nodes=" << primal.numNodes()
         << " edges=" << primal.numEdges()
         << " density=" << density
         << " edge/var=" << edge_var_ratio << endl;
  }

  if (primal.numEdges() > conf.td_max_edges) {
    cerr << "c too many edges, skipping TD" << endl; return 0;
  }
  if (density > conf.td_max_density) {
    cerr << "c density too high, skipping TD" << endl; return 0;
  }
  if (edge_var_ratio > conf.td_max_edge_var_ratio) {
    cerr << "c edge/var ratio too high, skipping TD" << endl; return 0;
  }

  if (!primal.isConnected()) {
    cerr << "c WARNING: primal graph is not connected" << endl;
  }

  TWD::IFlowCutter fc(primal.numNodes(), primal.numEdges(), conf.verb);
  fc.importGraph(primal);
  auto td = fc.constructTD(conf.td_steps, conf.td_iters);
  const int tw = td.width();
  if (conf.verb >= 1) cout << "c TD width: " << tw << endl;

  // per-variable score via centroid-distance (compute_td_score_using_raw path)
  td.centroid(conf.verb);
  auto dists = td.distanceFromCentroid();
  vector<double> tdscore(nvars, 0.0);
  if (!dists.empty()) {
    int max_dist = *std::max_element(dists.begin(), dists.end());
    if (max_dist > 0) {
      for (int i = 0; i < nvars && i < (int)dists.size(); i++)
        tdscore[i] = 100.0 * (double)(max_dist - dists[i]) / (double)max_dist;
    }
  }

  for (int i = 0; i < nvars; i++)
    cout << (i + 1) << " " << tdscore[i] << "\n";

  return 0;
}
