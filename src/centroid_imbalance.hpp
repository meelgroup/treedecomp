/******************************************
Shared "most central bag" (centroid) computation for tree decompositions.

Both TWD::TreeDecomposition and sspp::TreeDecomposition store their bags as a
sorted vector<vector<int>> and expose a per-bag neighbour list, so the centroid
logic lives here once and is parameterised on how neighbours are fetched.
***********************************************/

#pragma once

#include <vector>
#include <algorithm>

namespace twd_centroid {
using std::vector;

// Rooting the decomposition at bag 0, returns the number of graph vertices
// "introduced" in the subtree of v (each vertex counted once, at the bag
// closest to the root that contains it). As a side effect, fills maxComp[v]
// with the largest vertex-component weight left when bag v is removed: the
// biggest child subtree, or the upward part (nVerts - subtree weight). Bags
// must be sorted (binary_search is used to test parent membership).
template<class NeighborsFn>
int imbalanceDfs(int v, int parent, const vector<vector<int>>& bags,
                 NeighborsFn&& neighbors, int nVerts, vector<int>& maxComp) {
  int intros = 0;
  for (int x : bags[v]) {
    const bool inParent = parent != -1 &&
        std::binary_search(bags[parent].begin(), bags[parent].end(), x);
    if (!inParent) intros++;
  }

  int largestChild = 0;
  for (int ch : neighbors(v)) {
    if (ch == parent) continue;
    const int childWeight =
        imbalanceDfs(ch, v, bags, neighbors, nVerts, maxComp);
    intros += childWeight;
    largestChild = std::max(largestChild, childWeight);
  }

  maxComp[v] = std::max(largestChild, nVerts - intros);
  return intros;
}

// Returns the most central bag: the one whose removal leaves the smallest
// largest-component (imbalance). If maxCompOut is non-null it receives the
// per-bag imbalance vector (handy for logging). This is the NEW behaviour.
template<class NeighborsFn>
int mostCentralBag(const vector<vector<int>>& bags, NeighborsFn&& neighbors,
                   int nVerts, vector<int>* maxCompOut = nullptr) {
  const int n = (int)bags.size();
  vector<int> maxComp(n, 0);
  imbalanceDfs(0, -1, bags, neighbors, nVerts, maxComp);

  int best = 0;
  for (int v = 1; v < n; v++)
    if (maxComp[v] < maxComp[best]) best = v;

  if (maxCompOut) *maxCompOut = std::move(maxComp);
  return best;
}

// Post-order DFS for the OLD behaviour: sets `centroid` to the first bag
// (deepest in post-order) whose subtree introduces >= nVerts/2 vertices, and
// returns the subtree's introduced count. Bags must be sorted.
template<class NeighborsFn>
int firstFoundDfs(int v, int parent, const vector<vector<int>>& bags,
                  NeighborsFn&& neighbors, int nVerts, int& centroid) {
  int intros = 0;
  for (int ch : neighbors(v)) {
    if (ch == parent) continue;
    intros += firstFoundDfs(ch, v, bags, neighbors, nVerts, centroid);
    if (centroid != -1) return intros;
  }
  for (int x : bags[v]) {
    const bool inParent = parent != -1 &&
        std::binary_search(bags[parent].begin(), bags[parent].end(), x);
    if (!inParent) intros++;
  }
  if (intros >= nVerts / 2) centroid = v;
  return intros;
}

// Returns the first bag whose subtree introduces >= nVerts/2 vertices (OLD
// behaviour).
template<class NeighborsFn>
int firstFoundCentroid(const vector<vector<int>>& bags, NeighborsFn&& neighbors,
                       int nVerts) {
  int centroid = -1;
  firstFoundDfs(0, -1, bags, neighbors, nVerts, centroid);
  return centroid;
}

// Dispatches between the new (most central) and old (first found) centroid.
template<class NeighborsFn>
int centroidBag(const vector<vector<int>>& bags, NeighborsFn&& neighbors,
                int nVerts, bool useNew, vector<int>* maxCompOut = nullptr) {
  return useNew ? mostCentralBag(bags, neighbors, nVerts, maxCompOut)
                : firstFoundCentroid(bags, neighbors, nVerts);
}

}  // namespace twd_centroid
