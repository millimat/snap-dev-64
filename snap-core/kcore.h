// TODO ROK, Jure included basic documentation, finalize reference doc

//#//////////////////////////////////////////////
/// K-Core decomposition of a network.
/// K-core is defined as a maximal subgraph of the original graph where every node points to at least K other nodes.
/// K-core is obtained by repeatedly deleting nodes of degree < K from the graph until no nodes of degree < K exist.
/// If the input graph is directed we treat it as undirected multigraph, i.e., we ignore the edge directions but there may be up to two edges between a pair of nodes.
/// See the kcores example (examples/kcores/kcores.cpp) for how to use the code.
/// For example: for (KCore(Graph); KCore.GetNextCore()!=0; ) { } will produce a sequence of K-cores for K=1...
template<class PGraph>
class TKCore {
private:
  PGraph Graph;
  TInt64H DegH;
  TInt64 CurK;
  TInt64V NIdV;
private:
  void Init();
public:
  TKCore(const PGraph& _Graph) : Graph(_Graph) { Init(); }
  /// Gets the currrent value of K.
  /// For every call of GetNextCore() the value of K increases by 1.
  int64 GetCurK() const { return CurK; }
  /// Returns the number of nodes in the next (K=K+1) core.
  /// The function starts with K=1-core and every time we call it it increases the value of K by 1 and
  /// generates the core. The function proceeds until GetCoreNodes() returns 0. Return value of the function
  /// is the size (the number of nodes) in the K-core (for the current value of K).
  int64 GetNextCore();
  /// Directly generates the core of order K. 
  /// The function has the same effect as calling GetNextCore() K times.
  int64 GetCoreK(const int64& K);
  /// Gets the number of nodes in the K-core (for the current value of K).
  int64 GetCoreNodes() const { return NIdV.Len(); }
  /// Gets the number of edges in the K-core (for the current value of K).
  int64 GetCoreEdges() const;
  /// Returns the IDs of the nodes in the current K-core.
  const TInt64V& GetNIdV() const { return NIdV; }
  /// Returrns the graph of the current K-core.
  PGraph GetCoreG() const { return TSnap::GetSubGraph(Graph, NIdV); }
};

template<class PGraph>
void TKCore<PGraph>::Init() {
  DegH.Gen(Graph->GetNodes());
  for (typename PGraph::TObj::TNodeI NI = Graph->BegNI(); NI < Graph->EndNI(); NI++) {
    //DegH.AddDat(NI.GetId(), NI.GetOutDeg());
    DegH.AddDat(NI.GetId(), NI.GetDeg());
  }
  CurK = 0;
}

template<class PGraph>
int64 TKCore<PGraph>::GetCoreEdges() const {
  int64 CoreEdges = 0;
  for (int64 k = DegH.FFirstKeyId(); DegH.FNextKeyId(k); ) {
    CoreEdges += DegH[k];
  }
  return CoreEdges/2;
}

template<class PGraph>
int64 TKCore<PGraph>::GetNextCore() {
  TInt64V DelV;
  int64 NDel=-1, AllDeg=0; //Pass=1;
  TExeTm ExeTm;
  CurK++;
  //printf("Get K-core: %d\n", CurK());
  while (NDel != 0) {
    NDel = 0;
    for (int64 k = DegH.FFirstKeyId(); DegH.FNextKeyId(k); ) {
      const int64 NId = DegH.GetKey(k);
      const int64 Deg = DegH[k];
      if (Deg < CurK) {
        const typename PGraph::TObj::TNodeI NI = Graph->GetNI(NId);
        for (int64 e = 0; e < NI.GetDeg(); e++) {
          const int64 n = NI.GetNbrNId(e);
          const int64 nk = DegH.GetKeyId(n);
          if (nk != -1) { DegH[nk] -= 1; } // mark node deleted
        }
        DegH.DelKeyId(k);
        NDel++;  AllDeg++;
      }
    }
    //printf("\r  pass %d]  %d deleted,  %d all deleted  [%s]", Pass++, NDel, AllDeg, ExeTm.GetStr());
  }
  //printf("\n");
  DegH.Defrag();
  DegH.GetKeyV(NIdV);
  NIdV.Sort();
  return NIdV.Len(); // all nodes in the current core
}

template<class PGraph>
int64 TKCore<PGraph>::GetCoreK(const int64& K) {
  Init();
  CurK = K-1;
  return GetNextCore();
}

/////////////////////////////////////////////////
// Snap
namespace TSnap {
/// Returns the K-core of a graph.
/// If the core of order K does not exist the function returns an empty graph.
template<class PGraph>
PGraph GetKCore(const PGraph& Graph, const int64& K) {
  TKCore<PGraph> KCore(Graph);
  KCore.GetCoreK(K);
  return TSnap::GetSubGraph(Graph, KCore.GetNIdV());
}

/// Returns the number of nodes in each core of order K (where K=0, 1, ...)
template<class PGraph>
int64 GetKCoreNodes(const PGraph& Graph, TIntPr64V& CoreIdSzV) {
  TKCore<PGraph> KCore(Graph);
  CoreIdSzV.Clr();
  CoreIdSzV.Add(TInt64Pr(0, Graph->GetNodes()));
  for (int64 i = 1; KCore.GetNextCore() > 0; i++) {
    CoreIdSzV.Add(TInt64Pr(i, KCore.GetCoreNodes()));
  }
  return KCore.GetCurK();
}

/// Returns the number of edges in each core of order K (where K=0, 1, ...)
template<class PGraph>
int64 GetKCoreEdges(const PGraph& Graph, TIntPr64V& CoreIdSzV) {
  TKCore<PGraph> KCore(Graph);
  CoreIdSzV.Clr();
  CoreIdSzV.Add(TInt64Pr(0, Graph->GetEdges()));
  for (int i = 1; KCore.GetNextCore() > 0; i++) {
    CoreIdSzV.Add(TInt64Pr(i, KCore.GetCoreEdges()));
  }
  return KCore.GetCurK();
}

} // namespace TSnap
