// Standalone tool: read a DIMACS CNF, build the primal graph, run FlowCutter,
// and print per-variable tree-decomposition scores. Mirrors the cutoffs and
// options used by ganak's counter.cpp so results can be compared directly.

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iomanip>
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
  int do_td_use_opt_indep = 1;

  int verb = 1;
  string input;
  string dot_file;
};

struct ParsedCnf {
  int nvars = 0;
  vector<vector<int>> cls;
  vector<int> indep;     // c p show 1 2 ... 0
  vector<int> optindep;  // c p optshow 1 2 ... 0
};

static void write_dot(const string& fname, TWD::TreeDecomposition& td, int centroid) {
  std::ofstream out(fname);
  if (!out) { cerr << "ERROR: cannot write '" << fname << "'" << endl; exit(1); }
  const auto& bags = td.Bags();
  const auto& adj = td.get_adj_list();
  out << "graph TD {\n";
  out << "  node [shape=box, fontname=\"Courier\"];\n";
  for (size_t i = 0; i < bags.size(); i++) {
    out << "  b" << i << " [label=\"bag " << i
        << ((int)i == centroid ? " (centroid)" : "")
        << "\\nsize " << bags[i].size() << "\\n{";
    for (size_t j = 0; j < bags[i].size(); j++) {
      if (j) out << ", ";
      out << (bags[i][j] + 1);
    }
    out << "}\"";
    if ((int)i == centroid) out << ", style=filled, fillcolor=\"#ffd27a\"";
    out << "];\n";
  }
  for (size_t i = 0; i < adj.size(); i++)
    for (int nb : adj[i])
      if ((int)i < nb) out << "  b" << i << " -- b" << nb << ";\n";
  out << "}\n";
}

static bool parse_lit_list_after(const string& line, const string& prefix,
                                 vector<int>& out) {
  if (line.size() < prefix.size()) return false;
  if (line.compare(0, prefix.size(), prefix) != 0) return false;
  std::istringstream iss(line.substr(prefix.size()));
  int v;
  while (iss >> v) {
    if (v == 0) break;
    out.push_back(v);
  }
  return true;
}

static ParsedCnf read_cnf(const string& fname) {
  std::istream* in = nullptr;
  std::ifstream fin;
  if (fname == "-" || fname.empty()) {
    in = &std::cin;
  } else {
    fin.open(fname);
    if (!fin) { cerr << "ERROR: cannot open '" << fname << "'" << endl; exit(1); }
    in = &fin;
  }

  ParsedCnf out;
  int ncls = 0;
  string line;
  bool got_header = false;
  while (std::getline(*in, line)) {
    if (line.empty()) continue;
    if (line[0] == 'c') {
      // Headers from ganak's dump_td_cnf:
      //   "c p optshow 1 2 ... 0"   (optional independent / projection set)
      //   "c p show    1 2 ... 0"   (independent set)
      if (parse_lit_list_after(line, "c p optshow ", out.optindep)) continue;
      if (parse_lit_list_after(line, "c p show ", out.indep)) continue;
      continue;
    }
    if (line[0] == 'p') {
      std::istringstream iss(line);
      string p, cnf;
      iss >> p >> cnf >> out.nvars >> ncls;
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
    if (!c.empty()) out.cls.push_back(std::move(c));
  }
  if (!got_header) { cerr << "ERROR: missing 'p cnf' header" << endl; exit(1); }
  return out;
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
  add_int("--tdcontract", conf.do_td_contract, "Contract non-projection vars (clique-elim) before running TD");
  add_int("--tdoptindep", conf.do_td_use_opt_indep, "Use 'c p optshow' (1) or 'c p show' (0) as projection set");
  add_int("-v", conf.verb, "Verbosity");
  program.add_argument("--tdvis")
      .action([&](const auto& a){ conf.dot_file = a; })
      .default_value(string(""))
      .help("Write tree decomposition as a Graphviz DOT file to this path");
  program.add_argument("input")
      .action([&](const auto& a){ conf.input = a; })
      .default_value(string("-"))
      .help("DIMACS CNF file (default: stdin)");

  try { program.parse_args(argc, argv); }
  catch (const std::exception& e) {
    cerr << e.what() << endl << program; return 1;
  }

  auto parsed = read_cnf(conf.input);
  const int nvars = parsed.nvars;
  if (conf.verb >= 1)
    cout << "c parsed nvars=" << nvars << " clauses=" << parsed.cls.size()
         << " indep=" << parsed.indep.size()
         << " optindep=" << parsed.optindep.size() << endl;

  if (nvars <= 0) { cerr << "ERROR: no variables" << endl; return 1; }
  if (nvars > conf.td_varlim) {
    cerr << "c too many vars (" << nvars << " > " << conf.td_varlim
         << "), skipping TD" << endl;
    return 0;
  }

  // Pick the projection set (mirrors ganak's `do_td_use_opt_indep`).
  // Ganak uses `nodes = opt_indep_support_end - 1` (i.e. the projection-set
  // size) and assumes vars `[1..nodes]` are the projection vars (Arjun
  // renumbers them to be first).
  const vector<int>& proj =
      (conf.do_td_use_opt_indep && !parsed.optindep.empty()) ? parsed.optindep
    : (!parsed.indep.empty()) ? parsed.indep
    : parsed.optindep; // empty -> nodes = nvars below
  int nodes = nvars;
  if (!proj.empty()) {
    nodes = (int)proj.size();
    // Verify proj is exactly {1, .., nodes}; otherwise contraction-by-index
    // does NOT match ganak's behavior (ganak relies on Arjun renumbering).
    vector<int> sorted_proj = proj;
    std::sort(sorted_proj.begin(), sorted_proj.end());
    bool consecutive = !sorted_proj.empty() && sorted_proj.front() == 1;
    for (size_t i = 0; consecutive && i < sorted_proj.size(); i++)
      if (sorted_proj[i] != (int)(i + 1)) consecutive = false;
    if (!consecutive) {
      cerr << "c WARNING: projection set is not {1.." << nodes
           << "}; contraction-by-index will NOT match ganak's behavior" << endl;
    }
  }
  if (conf.verb >= 1)
    cout << "c projection nodes=" << nodes
         << " (using " << (conf.do_td_use_opt_indep ? "optshow" : "show") << ")"
         << endl;

  auto primal = build_primal(nvars, parsed.cls);
  if (conf.verb >= 1) {
    cout << "c raw primal nodes=" << primal.numNodes()
         << " edges=" << primal.numEdges() << endl;
  }

  // Mirror ganak's td_decompose contraction loop: contract every var index
  // i in [nodes, nvars), bailing early if edge count blows past max_edges*100.
  if (conf.do_td_contract && nodes < nvars) {
    const int contract_cap = conf.td_max_edges * 100;
    int contracted = 0;
    for (int i = nodes; i < nvars; i++) {
      primal.contract(i, contract_cap);
      contracted++;
      if (primal.numEdges() > contract_cap) {
        if (conf.verb >= 1)
          cerr << "c contraction blew edge budget after " << contracted
               << " vars (edges=" << primal.numEdges()
               << " > " << contract_cap << "), stopping early" << endl;
        break;
      }
    }
    if (conf.verb >= 1)
      cout << "c after contraction: edges=" << primal.numEdges()
           << " (contracted " << contracted << " vars)" << endl;
  }

  const uint64_t n2 = (uint64_t)nvars * (uint64_t)nvars;
  const double density = n2 ? (double)primal.numEdges() / (double)n2 : 0.0;
  const double edge_var_ratio = nvars ? (double)primal.numEdges() / (double)nvars : 0.0;
  if (conf.verb >= 1) {
    cout << "c primal (post-contract) edges=" << primal.numEdges()
         << " density=" << std::fixed << std::setprecision(4) << density
         << " edge/var=" << std::fixed << std::setprecision(3) << edge_var_ratio
         << std::resetiosflags(std::ios::fixed) << endl;
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

  // After contraction, contracted vars are isolated. Project to a graph of
  // exactly `nodes` vertices to match ganak's `primal_alt`.
  TWD::Graph primal_alt;
  if (conf.do_td_contract && nodes < nvars) {
    primal_alt = TWD::Graph(nodes);
    const auto& adj = primal.get_adj_list();
    for (int i = 0; i < nodes; i++) {
      for (int nb : adj[i])
        if (nb < nodes) primal_alt.addEdge(i, nb);
    }
  } else {
    primal_alt = primal;
  }
  if (conf.verb >= 1) {
    cout << "c primal_alt nodes=" << primal_alt.numNodes()
         << " edges=" << primal_alt.numEdges() << endl;
  }

  if (!primal_alt.isConnected()) {
    cerr << "c WARNING: primal graph is not connected" << endl;
  }

  TWD::IFlowCutter fc(primal_alt.numNodes(), primal_alt.numEdges(), conf.verb);
  fc.importGraph(primal_alt);
  auto td = fc.constructTD(conf.td_steps, conf.td_iters);
  const int tw = td.width();
  if (conf.verb >= 1) cout << "c TD width: " << tw << endl;

  const int centroid = td.centroid(conf.verb);
  if (conf.verb >= 1) cout << "c centroid bag: " << centroid << endl;

  if (!conf.dot_file.empty()) {
    write_dot(conf.dot_file, td, centroid);
    if (conf.verb >= 1)
      cout << "c wrote DOT file: " << conf.dot_file
           << " (render: dot -Tpdf " << conf.dot_file << " -o td.pdf)" << endl;
  }

  // per-variable score via centroid-distance (compute_td_score_using_raw path)
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
