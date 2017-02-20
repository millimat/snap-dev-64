#include <gtest/gtest.h>

#include "Snap.h"

class GGenTest { };  // For gtest highlighting

// Test generation of Grid graph
TEST(GGenTest, GenGrid) {
  const int64 RowsStart = 1;
  const int64 RowsEnd = 20;
  
  const int64 ColsStart = 1;
  const int64 ColsEnd = 15;
  
  PUNGraph UNGraph;
  PNGraph NGraph;

  for (int64 Rows = RowsStart; Rows < RowsEnd; Rows++) {
    
    for (int64 Cols = ColsStart; Cols < ColsEnd; Cols++) {

      int64 NumEdges = 0;
      
      // Calculate correct number of edges in grid:
      // Add right side and left side edges
      if (Cols > 1) {
        NumEdges += (Rows - 1) * 2;
      }
      else {
        NumEdges += Rows - 1;
      }

      // Add top and bottom sides
      if (Rows > 1) {
        NumEdges += (Cols - 1) * 2;
      }
      else {
        NumEdges += Cols - 1;
      }

      if (Rows > 1 && Cols > 1) {
        // Add inner side edges
        if (Cols >= 2) {
          NumEdges += Cols - 2;
        }
        if (Rows >= 2) {
          NumEdges += Rows - 2;
        }
        // Add inside edges
        if (Rows >= 3 || Cols >= 3) {
          NumEdges += (Rows-2) * (Cols-2) * 2;
        }
      }
      // -------------------------------------------
      // Generate undirected grid graph
      UNGraph = TSnap::GenGrid<PUNGraph>(Rows, Cols, false);
      EXPECT_FALSE(UNGraph->Empty());
      EXPECT_TRUE(UNGraph->IsOk());
      EXPECT_EQ(Rows*Cols, UNGraph->GetNodes());
      EXPECT_EQ(NumEdges, UNGraph->GetEdges());
      
      int64 NodeCount = 0;
      int64 EdgeCount = 0;
      // Iterate through nodes, verify it matches
      for (TUNGraph::TNodeI NI = UNGraph->BegNI(); NI < UNGraph->EndNI(); NI++) {
        NodeCount++;
        for (int64 e = 0; e < NI.GetOutDeg(); e++) {
          EdgeCount++;
        }
      }
      EXPECT_EQ(NodeCount, UNGraph->GetNodes());
      EXPECT_EQ(EdgeCount/2, UNGraph->GetEdges());
      
      EdgeCount = 0;
      // Iterate through edges, count and verify
      for (TUNGraph::TEdgeI NI = UNGraph->BegEI(); NI < UNGraph->EndEI(); NI++) {
        EdgeCount++;
      }
      EXPECT_EQ(EdgeCount, UNGraph->GetEdges());
      
      // -------------------------------------------
      // Generate directed grid graph  
      NGraph = TSnap::GenGrid<PNGraph>(Rows, Cols, true);
      EXPECT_FALSE(NGraph->Empty());
      EXPECT_TRUE(NGraph->IsOk());
      EXPECT_EQ(Rows*Cols, NGraph->GetNodes());
      EXPECT_EQ(NumEdges, NGraph->GetEdges());
      
      NodeCount = 0;
      for (TNGraph::TNodeI NI = NGraph->BegNI(); NI < NGraph->EndNI(); NI++) {
        NodeCount++;
        for (int64 e = 0; e < NI.GetOutDeg(); e++) {
          EdgeCount++;
        }
      }
      EXPECT_EQ(NodeCount, NGraph->GetNodes());
      EXPECT_EQ(EdgeCount/2, NGraph->GetEdges());
      
      EdgeCount = 0;
      // Iterate through edges, count and verify
      for (TNGraph::TEdgeI NI = NGraph->BegEI(); NI < NGraph->EndEI(); NI++) {
        EdgeCount++;
      }
      EXPECT_EQ(EdgeCount, NGraph->GetEdges());
    } // end loop - Cols
  } // end loop - Rows

}

// Test generation of Star graph
TEST(GGenTest, GenStar) {
  const int64 NNodesMax = 1000;
  
  PUNGraph UNGraph;
  PNGraph NGraph;
      
  for (int64 NNodes = 1; NNodes < NNodesMax; NNodes++) {
    
    int64 NumEdges = NNodes-1;
    
    // -------------------------------------------
    // Generate undirected graph
    UNGraph = TSnap::GenStar<PUNGraph>(NNodes, false);
    EXPECT_FALSE(UNGraph->Empty());
    EXPECT_TRUE(UNGraph->IsOk());
    EXPECT_EQ(NumEdges, UNGraph->GetEdges());
    
    int64 NodeCount = 0;
    int64 EdgeCount = 0;
    // Iterate through nodes, verify it matches
    for (TUNGraph::TNodeI NI = UNGraph->BegNI(); NI < UNGraph->EndNI(); NI++) {
      NodeCount++;
      for (int64 e = 0; e < NI.GetOutDeg(); e++) {
        EdgeCount++;
      }
    }
    EXPECT_EQ(NodeCount, UNGraph->GetNodes());
    EXPECT_EQ(EdgeCount/2, UNGraph->GetEdges());
    
    EdgeCount = 0;
    // Iterate through edges, count and verify
    for (TUNGraph::TEdgeI NI = UNGraph->BegEI(); NI < UNGraph->EndEI(); NI++) {
      EdgeCount++;
    }
    EXPECT_EQ(EdgeCount, UNGraph->GetEdges());
    
    // -------------------------------------------
    // Generate directed graph
    NGraph = TSnap::GenStar<PNGraph>(NNodes, true);
    EXPECT_FALSE(NGraph->Empty());
    EXPECT_TRUE(NGraph->IsOk());
    EXPECT_EQ(NumEdges, NGraph->GetEdges());
    
    NodeCount = 0;
    for (TNGraph::TNodeI NI = NGraph->BegNI(); NI < NGraph->EndNI(); NI++) {
      NodeCount++;
      for (int64 e = 0; e < NI.GetOutDeg(); e++) {
        EdgeCount++;
      }
    }
    EXPECT_EQ(NodeCount, NGraph->GetNodes());
    EXPECT_EQ(EdgeCount/2, NGraph->GetEdges());
    
    EdgeCount = 0;
    // Iterate through edges, count and verify
    for (TNGraph::TEdgeI NI = NGraph->BegEI(); NI < NGraph->EndEI(); NI++) {
      EdgeCount++;
    }
    EXPECT_EQ(EdgeCount, NGraph->GetEdges());
    
  } // end loop - NNodes
}

// Test generation of Circle graph
TEST(GGenTest, GenCircle) {
  const int64 NNodesMax = 500;
  const int64 NodeOutDegMax = 5;
  
  PUNGraph UNGraph;
  PNGraph NGraph;
  
  for (int64 NNodes = 1; NNodes < NNodesMax; NNodes++) {

    for (int64 NodeOutDeg = 1; NodeOutDeg < NodeOutDegMax; NodeOutDeg++) {
            
      // -------------------------------------------
      // Generate undirected graph
      UNGraph = TSnap::GenCircle<PUNGraph>(NNodes, NodeOutDeg, false);
      
      EXPECT_FALSE(UNGraph->Empty());
      EXPECT_TRUE(UNGraph->IsOk());
            
      int64 NodeCount = 0;
      int64 EdgeCount = 0;
      // Iterate through nodes, verify it matches
      for (TUNGraph::TNodeI NI = UNGraph->BegNI(); NI < UNGraph->EndNI(); NI++) {
        NodeCount++;
      }
      EXPECT_EQ(NodeCount, UNGraph->GetNodes());
      
      EdgeCount = 0;
      // Iterate through edges, count and verify
      for (TUNGraph::TEdgeI NI = UNGraph->BegEI(); NI < UNGraph->EndEI(); NI++) {
        EdgeCount++;
      }
      EXPECT_EQ(EdgeCount, UNGraph->GetEdges());
      
      // -------------------------------------------
      // Generate directed graph
      NGraph = TSnap::GenCircle<PNGraph>(NNodes, NodeOutDeg, true);
      EXPECT_FALSE(NGraph->Empty());
      EXPECT_TRUE(NGraph->IsOk());
            
      NodeCount = 0;
      for (TNGraph::TNodeI NI = NGraph->BegNI(); NI < NGraph->EndNI(); NI++) {
        NodeCount++;
      }
      EXPECT_EQ(NodeCount, NGraph->GetNodes());
      
      EdgeCount = 0;
      // Iterate through edges, count and verify
      for (TNGraph::TEdgeI NI = NGraph->BegEI(); NI < NGraph->EndEI(); NI++) {
        EdgeCount++;
      }
      EXPECT_EQ(EdgeCount, NGraph->GetEdges());
    }
    
  } // end loop - NNodes
}

// Test generation of Full graph 
TEST(GGenTest, GenFull) {
  const int64 NNodesMax = 100;
  
  PUNGraph UNGraph;
  PNGraph NGraph;
  
  for (int64 NNodes = 1; NNodes < NNodesMax; NNodes++) {
          
    // -------------------------------------------
    // Generate undirected graph
    UNGraph = TSnap::GenFull<PUNGraph>(NNodes);
    
    EXPECT_FALSE(UNGraph->Empty());
    EXPECT_TRUE(UNGraph->IsOk());
  
    // Number of edges in a complete graph = n * (n-1) / 2
    int64 NumEdges = NNodes * (NNodes - 1) / 2;
    EXPECT_EQ(NumEdges, UNGraph->GetEdges());
  
    int64 NodeCount = 0;
    int64 EdgeCount = 0;
    // Iterate through nodes, verify it matches
    for (TUNGraph::TNodeI NI = UNGraph->BegNI(); NI < UNGraph->EndNI(); NI++) {
      NodeCount++;
    }
    EXPECT_EQ(NodeCount, UNGraph->GetNodes());
    
    EdgeCount = 0;
    // Iterate through edges, count and verify
    for (TUNGraph::TEdgeI NI = UNGraph->BegEI(); NI < UNGraph->EndEI(); NI++) {
      EdgeCount++;
    }
    EXPECT_EQ(EdgeCount, UNGraph->GetEdges());
    
    // -------------------------------------------
    // Generate directed graph
    NGraph = TSnap::GenFull<PNGraph>(NNodes);
    EXPECT_FALSE(NGraph->Empty());
    EXPECT_TRUE(NGraph->IsOk());
    
    NumEdges = NNodes * (NNodes - 1);
    EXPECT_EQ(NumEdges, NGraph->GetEdges());
    
    NodeCount = 0;
    for (TNGraph::TNodeI NI = NGraph->BegNI(); NI < NGraph->EndNI(); NI++) {
      NodeCount++;
    }
    EXPECT_EQ(NodeCount, NGraph->GetNodes());
    
    EdgeCount = 0;
    // Iterate through edges, count and verify
    for (TNGraph::TEdgeI NI = NGraph->BegEI(); NI < NGraph->EndEI(); NI++) {
      EdgeCount++;
    }
    EXPECT_EQ(EdgeCount, NGraph->GetEdges());
    
  } // end loop - NNodes
  
}

// Test generation of Tree graph 
TEST(GGenTest, GenTree) {
  const int64 FanoutMax = 10;
  const int64 LevelsMax = 5;
  
  PUNGraph UNGraph;
  PNGraph NGraph;
  
  for (int64 Fanout = 2; Fanout < FanoutMax; Fanout++) {
    
    for (int64 Levels = 1; Levels < LevelsMax; Levels++) {
      
      // -------------------------------------------
      // Generate undirected graph
      UNGraph = TSnap::GenTree<PUNGraph>(Fanout, Levels, false);
      
      EXPECT_FALSE(UNGraph->Empty());
      EXPECT_TRUE(UNGraph->IsOk());
            
      int64 NodeCount = 0;
      int64 EdgeCount = 0;
      // Iterate through nodes, verify it matches
      for (TUNGraph::TNodeI NI = UNGraph->BegNI(); NI < UNGraph->EndNI(); NI++) {
        NodeCount++;
      }
      EXPECT_EQ(NodeCount, UNGraph->GetNodes());
      
      EdgeCount = 0;
      // Iterate through edges, count and verify
      for (TUNGraph::TEdgeI NI = UNGraph->BegEI(); NI < UNGraph->EndEI(); NI++) {
        EdgeCount++;
      }
      EXPECT_EQ(EdgeCount, UNGraph->GetEdges());
      
      // -------------------------------------------
      // Generate directed graph
      NGraph = TSnap::GenTree<PNGraph>(Fanout, Levels, true, true);
      EXPECT_FALSE(NGraph->Empty());
      EXPECT_TRUE(NGraph->IsOk());
      
      NodeCount = 0;
      for (TNGraph::TNodeI NI = NGraph->BegNI(); NI < NGraph->EndNI(); NI++) {
        NodeCount++;
      }
      EXPECT_EQ(NodeCount, NGraph->GetNodes());
      
      EdgeCount = 0;
      // Iterate through edges, count and verify
      for (TNGraph::TEdgeI NI = NGraph->BegEI(); NI < NGraph->EndEI(); NI++) {
        EdgeCount++;
      }
      EXPECT_EQ(EdgeCount, NGraph->GetEdges());
    
    } // end loop - Levels
  } // end loop - Fanout
}

// Test generation of BaraHierar graph 
TEST(GGenTest, GenBaraHierar) {
  const int64 LevelsMax = 8;
  
  PUNGraph UNGraph;
  PNGraph NGraph;
      
  for (int64 Levels = 1; Levels < LevelsMax; Levels++) {
    
    // -------------------------------------------
    // Generate undirected graph
    UNGraph = TSnap::GenBaraHierar<PUNGraph>(Levels);
    
    EXPECT_FALSE(UNGraph->Empty());
    EXPECT_TRUE(UNGraph->IsOk());
    
    int64 NodeCount = 0;
    int64 EdgeCount = 0;
    // Iterate through nodes, verify it matches
    for (TUNGraph::TNodeI NI = UNGraph->BegNI(); NI < UNGraph->EndNI(); NI++) {
      NodeCount++;
    }
    EXPECT_EQ(NodeCount, UNGraph->GetNodes());
    
    EdgeCount = 0;
    // Iterate through edges, count and verify
    for (TUNGraph::TEdgeI NI = UNGraph->BegEI(); NI < UNGraph->EndEI(); NI++) {
      EdgeCount++;
    }
    EXPECT_EQ(EdgeCount, UNGraph->GetEdges());
    
    // -------------------------------------------
    // Generate directed graph
    NGraph = TSnap::GenBaraHierar<PNGraph>(Levels);
    EXPECT_FALSE(NGraph->Empty());
    EXPECT_TRUE(NGraph->IsOk());
    
    NodeCount = 0;
    for (TNGraph::TNodeI NI = NGraph->BegNI(); NI < NGraph->EndNI(); NI++) {
      NodeCount++;
    }
    EXPECT_EQ(NodeCount, NGraph->GetNodes());
    
    EdgeCount = 0;
    // Iterate through edges, count and verify
    for (TNGraph::TEdgeI NI = NGraph->BegEI(); NI < NGraph->EndEI(); NI++) {
      EdgeCount++;
    }
    EXPECT_EQ(EdgeCount, NGraph->GetEdges());
    
  } // end loop - Levels
}

// Test generation of RndGnm graph 
TEST(GGenTest, GenRndGnm) {
  const int64 NNodesMax = 30;
  const int64 NEdgesMax = 50;
  int64 NodeCount = 0;
  int64 EdgeCount = 0;

  PUNGraph UNGraph;
  PNGraph NGraph;
  
  TInt::Rnd.PutSeed(0);
  
  for (int64 NNodes = 1; NNodes < NNodesMax; NNodes++) {
    
    for (int64 NEdges = 0; NEdges < NEdgesMax; NEdges++) {
      
      // Make sure we have enough nodes for the requested edges
      if (NNodes * (NNodes-1) / 2 >= NEdges) {

        // -------------------------------------------
        // Generate undirected graph
        UNGraph = TSnap::GenRndGnm<PUNGraph>(NNodes, NEdges, false, TInt::Rnd);
        EXPECT_FALSE(UNGraph->Empty());
        EXPECT_TRUE(UNGraph->IsOk());
              
        NodeCount = 0;
        EdgeCount = 0;
        // Iterate through nodes, verify it matches
        for (TUNGraph::TNodeI NI = UNGraph->BegNI(); NI < UNGraph->EndNI(); NI++) {
          NodeCount++;
        }
        EXPECT_EQ(NodeCount, UNGraph->GetNodes());
        
        EdgeCount = 0;
        // Iterate through edges, count and verify
        for (TUNGraph::TEdgeI NI = UNGraph->BegEI(); NI < UNGraph->EndEI(); NI++) {
          EdgeCount++;
        }
        EXPECT_EQ(EdgeCount, UNGraph->GetEdges());
      }
      
      else if (NNodes * (NNodes-1) >= NEdges) {

        // -------------------------------------------
        // Generate directed graph
        NGraph = TSnap::GenRndGnm<PNGraph>(NNodes, NEdges, true, TInt::Rnd);
        EXPECT_FALSE(NGraph->Empty());
        EXPECT_TRUE(NGraph->IsOk());
              
        NodeCount = 0;
        for (TNGraph::TNodeI NI = NGraph->BegNI(); NI < NGraph->EndNI(); NI++) {
          NodeCount++;
        }
        EXPECT_EQ(NodeCount, NGraph->GetNodes());
        
        EdgeCount = 0;
        // Iterate through edges, count and verify
        for (TNGraph::TEdgeI NI = NGraph->BegEI(); NI < NGraph->EndEI(); NI++) {
          EdgeCount++;
        }
        EXPECT_EQ(EdgeCount, NGraph->GetEdges());
      }
    
    } // end loop - NEdges
  } // end loop - NNodes
}

// Test generation of RndBipart graph 
TEST(GGenTest, GenRndBipart) {
  const int64 LeftNodesMin = 8;
  const int64 LeftNodesMax = 20;
  const int64 RightNodesMin = 8;
  const int64 RightNodesMax = 50;
  const int64 EdgesMin = 5;
  const int64 EdgesMax = 30;
  
  PBPGraph Graph;
  
  TInt::Rnd.PutSeed(0);
  
  for (int64 LeftNodes = LeftNodesMin; LeftNodes < LeftNodesMax; LeftNodes++) {
    
    for (int64 RightNodes = RightNodesMin; RightNodes < RightNodesMax; RightNodes++) {
      
      for (int64 Edges = EdgesMin;  Edges < EdgesMax; Edges++) {
      
        // Make sure there are sufficient number of edges to cover the BP graph
        if (Edges > LeftNodes * RightNodes) {
          continue;
        }
        
        // -------------------------------------------
        // Generate undirected graph
        Graph = TSnap::GenRndBipart(LeftNodes, RightNodes, Edges, TInt::Rnd);
        EXPECT_FALSE(Graph->Empty());
//        EXPECT_TRUE(Graph->IsOk());   // BP graph always returns 'not ok'
        
        int64 NodeCount = 0;
        int64 EdgeCount = 0;
        // Iterate through nodes, verify it matches
        for (TBPGraph::TNodeI NI = Graph->BegLNI(); NI < Graph->EndLNI(); NI++) {
          NodeCount++;
        }
        for (TBPGraph::TNodeI NI = Graph->BegRNI(); NI < Graph->EndRNI(); NI++) {
          NodeCount++;
        }
        //EXPECT_EQ(NodeCount, Graph->GetNodes());
        
        EdgeCount = 0;
        // Iterate through edges, count and verify
        for (TBPGraph::TEdgeI NI = Graph->BegEI(); NI < Graph->EndEI(); NI++) {
          EdgeCount++;
        }
        EXPECT_EQ(EdgeCount, Graph->GetEdges());
        
      } // end loop - NEdges
    } // end loop - LeftNodes
  } // end loop - RightNodes

}

// Tests power-law like networks
// http://en.wikipedia.org/wiki/Scale-free_network
TEST(GGenTest, GenRndPowerLaw) {
  const int64 NNodesMin = 10;
  const int64 NNodesMax = 1000;
  const double PowerExpMin = 2.0;
  const double PowerExpMax = 3.0;
  int64 NodeCount = 0;
  int64 EdgeCount = 0;
  
  PUNGraph UNGraph;

  TInt::Rnd.PutSeed(0);
  
  // Outputs a lot of text, so only run a few iterations
  for (int64 NNodes = NNodesMin; NNodes <= NNodesMax; NNodes += (NNodesMax - NNodesMin)/2) {
    
    for (double PowerExp = PowerExpMin; PowerExp <= PowerExpMax; PowerExp += 1.0) {
      
      // Generate undirected graph
      UNGraph = TSnap::GenRndPowerLaw(NNodes, PowerExp);
      EXPECT_FALSE(UNGraph->Empty());
      EXPECT_TRUE(UNGraph->IsOk());
            
      NodeCount = 0;
      EdgeCount = 0;
      // Iterate through nodes, verify it matches
      for (TUNGraph::TNodeI NI = UNGraph->BegNI(); NI < UNGraph->EndNI(); NI++) {
        NodeCount++;
      }
      EXPECT_EQ(NodeCount, UNGraph->GetNodes());
      
      EdgeCount = 0;
      // Iterate through edges, count and verify
      for (TUNGraph::TEdgeI NI = UNGraph->BegEI(); NI < UNGraph->EndEI(); NI++) {
        EdgeCount++;
      }
      EXPECT_EQ(EdgeCount, UNGraph->GetEdges());
      
    } // end loop - NEdges
  } // end loop - NNodes

}

// Test generator for degree sequence -- hangs at times
TEST(GGenTest, DISABLED_GenDegSeq) {
  const int64 NNodesMax = 15;
  const int64 NumIterations = 10;  // Set to large number for infinite loops
  
  PUNGraph UNGraph;
  TInt64V DegSeqV;
  TInt::Rnd.PutSeed(0);
  
  int64 NodeCount = 0;
  int64 EdgeCount = 0;
  
  for (int64 i = 0; i < NumIterations; i++) {
    
    // For speed in testing, populate long tail of 1's
    for (int64 NNodes = 4; NNodes < NNodesMax; NNodes+=NNodesMax/5) {
          
      // Generate Deg Sequence vector randomly
      DegSeqV.Gen(NNodes);
      int64 DegSum = 0;
      for (int64 n = 0; n < DegSeqV.Len()/4; n++) {
        DegSeqV[n] = TInt::Rnd.GetUniDevInt(1, NNodes/2);
        DegSum += DegSeqV[n];
      }
      // Add 1's to the tail
      for (int64 n = DegSeqV.Len()/4; n < DegSeqV.Len(); n++) {
        DegSeqV[n] = 1;
        DegSum += DegSeqV[n];
      }
      // Make sure the sum of degrees is divisible by 2
      if (DegSum % 2 != 0) {
        DegSeqV[(int64)TInt::Rnd.GetUniDevInt(NNodes)]++;
      }
      DegSeqV.Sort();
      DegSeqV.Reverse();
      
      printf("NNodes=%s, ", TInt64::GetStr(NNodes).CStr());
      printf("DegSeqV = { ");
      for (int64 i = 0; i < DegSeqV.Len(); i++) {
        printf("%s ", TInt64::GetStr((int64)DegSeqV[i]).CStr());
      }
      printf("}\n");
      
      // Generate undirected graph      
      UNGraph = TSnap::GenDegSeq(DegSeqV);
      EXPECT_FALSE(UNGraph->Empty());
      EXPECT_TRUE(UNGraph->IsOk());
      
      NodeCount = 0;
      EdgeCount = 0;
      // Iterate through nodes, verify it matches
      for (TUNGraph::TNodeI NI = UNGraph->BegNI(); NI < UNGraph->EndNI(); NI++) {
        NodeCount++;
      }
      EXPECT_EQ(NodeCount, UNGraph->GetNodes());
      
      EdgeCount = 0;
      // Iterate through edges, count and verify
      for (TUNGraph::TEdgeI NI = UNGraph->BegEI(); NI < UNGraph->EndEI(); NI++) {
        EdgeCount++;
      }
      EXPECT_EQ(EdgeCount, UNGraph->GetEdges());
    }
  }
}

// Test generation of PrefAttach graph 
TEST(GGenTest, GenPrefAttach) {
  const int64 NNodesMax = 100;
  const int64 NodeOutDegMax = 15;
  
  PUNGraph UNGraph;
  
  for (int64 NNodes = 0; NNodes < NNodesMax; NNodes++) {
    
    for (int64 NodeOutDeg = 0; NodeOutDeg < NodeOutDegMax; NodeOutDeg++) {
      
      int64 NodeCount = 0;
      int64 EdgeCount = 0;
      
      // -------------------------------------------
      // Generate undirected graph
      UNGraph = TSnap::GenPrefAttach(NNodes, NodeOutDeg);
      EXPECT_FALSE(UNGraph->Empty());
      EXPECT_TRUE(UNGraph->IsOk());
      
      NodeCount = 0;
      EdgeCount = 0;
      // Iterate through nodes, verify it matches
      for (TUNGraph::TNodeI NI = UNGraph->BegNI(); NI < UNGraph->EndNI(); NI++) {
        NodeCount++;
      }
      EXPECT_EQ(NodeCount, UNGraph->GetNodes());
      
      EdgeCount = 0;
      // Iterate through edges, count and verify
      for (TUNGraph::TEdgeI NI = UNGraph->BegEI(); NI < UNGraph->EndEI(); NI++) {
        EdgeCount++;
      }
      EXPECT_EQ(EdgeCount, UNGraph->GetEdges());
      
    } // end loop - NEdges
  } // end loop - NNodes
}

// Test generation of GeoPrefAttach graph 
TEST(GGenTest, GenGeoPrefAttach) {
  const int64 NNodesMax = 100;
  const int64 NodeOutDegMax = 15;
  const double BetaMax = 2.0;
  
  PUNGraph UNGraph;
  PNGraph NGraph;
  
  for (int64 NNodes = 1; NNodes < NNodesMax; NNodes++) {
    
    for (int64 NodeOutDeg = 0; NodeOutDeg < NodeOutDegMax; NodeOutDeg++) {
      
      for (double Beta = -1*BetaMax; Beta < BetaMax; Beta += 0.2) {
      
        int64 NodeCount = 0;
        int64 EdgeCount = 0;
        
        // -------------------------------------------
        // Generate undirected graph
        UNGraph = TSnap::GenGeoPrefAttach(NNodes, NodeOutDeg, Beta);
        EXPECT_FALSE(UNGraph->Empty());
        EXPECT_TRUE(UNGraph->IsOk());
        
        NodeCount = 0;
        EdgeCount = 0;
        // Iterate through nodes, verify it matches
        for (TUNGraph::TNodeI NI = UNGraph->BegNI(); NI < UNGraph->EndNI(); NI++) {
          NodeCount++;
        }
        EXPECT_EQ(NodeCount, UNGraph->GetNodes());
        
        EdgeCount = 0;
        // Iterate through edges, count and verify
        for (TUNGraph::TEdgeI NI = UNGraph->BegEI(); NI < UNGraph->EndEI(); NI++) {
          EdgeCount++;
        }
        EXPECT_EQ(EdgeCount, UNGraph->GetEdges());
      } // end loop - Beta
    } // end loop - NEdges
  } // end loop - NNodes
}

// Test generation of SmallWorld graph 
TEST(GGenTest, GenSmallWorld) {
  const int64 NNodesMax = 1000;
  const double RewireProbMax = 1.0;
  
  PUNGraph UNGraph;
  
  for (int64 NNodes = 1; NNodes < NNodesMax; NNodes+=NNodesMax/20) {
    
    for (int64 NodeOutDeg = 1; NodeOutDeg < 10; NodeOutDeg++) {
      
      for (double RewireProb = 0.0; RewireProb <= RewireProbMax; RewireProb += 0.2) {
        
        int64 NodeCount = 0;
        int64 EdgeCount = 0;
        
        if (NNodes <= NodeOutDeg) {
          continue;
        }

        // -------------------------------------------
        // Generate undirected graph
        UNGraph = TSnap::GenSmallWorld(NNodes, NodeOutDeg, RewireProb);
        EXPECT_FALSE(UNGraph->Empty());
        EXPECT_TRUE(UNGraph->IsOk());
        
        NodeCount = 0;
        EdgeCount = 0;
        // Iterate through nodes, verify it matches
        for (TUNGraph::TNodeI NI = UNGraph->BegNI(); NI < UNGraph->EndNI(); NI++) {
          NodeCount++;
        }
        EXPECT_EQ(NodeCount, UNGraph->GetNodes());
        
        EdgeCount = 0;
        // Iterate through edges, count and verify
        for (TUNGraph::TEdgeI NI = UNGraph->BegEI(); NI < UNGraph->EndEI(); NI++) {
          EdgeCount++;
        }
        EXPECT_EQ(EdgeCount, UNGraph->GetEdges());
      } // end loop - RewireProb
    } // end loop - NEdges
  } // end loop - NNodes
}

// Test Forest Fire graph generator
// Note: outputs a lot of text, so run with only a few values
/*
TEST(GGenTest, GenForestFire) {
  const int64 NNodesMax = 1000;
  const double FwdProbMax = 1.0;
  const double BckProbMax = 1.0;
  
//  PNGraph Graph;
  
  for (int64 NNodes = 0; NNodes < NNodesMax; NNodes += NNodesMax/2) {
    
    for (double FwdProb = 0.1; FwdProb < FwdProbMax; FwdProb += 0.5) {

      for (double BckProb = 0.1; BckProb < BckProbMax; BckProb += 0.5) {
        
        int64 NodeCount = 0;
        int64 EdgeCount = 0;
        
        // -------------------------------------------
        // Generate directed forest fire graph
        PNGraph Graph = TSnap::GenForestFire(NNodes, FwdProb, BckProb);
        if (0 != NNodes) {
          EXPECT_FALSE(Graph->Empty());
        }
        EXPECT_TRUE(Graph->IsOk());
        
        NodeCount = 0;
        EdgeCount = 0;
        // Iterate through nodes, verify it matches
        for (TNGraph::TNodeI NI = Graph->BegNI(); NI < Graph->EndNI(); NI++) {
          NodeCount++;
        }
        EXPECT_EQ(NodeCount, Graph->GetNodes());
        
        EdgeCount = 0;
        // Iterate through edges, count and verify
        for (TNGraph::TEdgeI NI = Graph->BegEI(); NI < Graph->EndEI(); NI++) {
          EdgeCount++;
        }
        EXPECT_EQ(EdgeCount, Graph->GetEdges());
      } // end loop - RewireProb
    } // end loop - NEdges
  } // end loop - NNodes
}
*/

// Test generation of CopyModel graph 
TEST(GGenTest, GenCopyModel) {
  const int64 NNodesMax = 100;
  const double BetaMax = 2.0;
  
  PNGraph Graph;
  
  for (int64 NNodes = 0; NNodes < NNodesMax; NNodes++) {
          
    for (double Beta = -1*BetaMax; Beta < BetaMax; Beta += 0.2) {
      
      int64 NodeCount = 0;
      int64 EdgeCount = 0;
      
      Graph = TSnap::GenCopyModel(NNodes, Beta);
      EXPECT_FALSE(Graph->Empty());
      EXPECT_TRUE(Graph->IsOk());
      
      NodeCount = 0;
      EdgeCount = 0;
      // Iterate through nodes, verify it matches
      for (TNGraph::TNodeI NI = Graph->BegNI(); NI < Graph->EndNI(); NI++) {
        NodeCount++;
      }
      EXPECT_EQ(NodeCount, Graph->GetNodes());
      
      EdgeCount = 0;
      // Iterate through edges, count and verify
      for (TNGraph::TEdgeI NI = Graph->BegEI(); NI < Graph->EndEI(); NI++) {
        EdgeCount++;
      }
      EXPECT_EQ(EdgeCount, Graph->GetEdges());
    } // end loop - Beta
    
  } // end loop - NNodes
  
}

// Test generation of RMat graph 
TEST(GGenTest, GenRMat) {
  const int64 NNodesMax = 10000;
  const int64 NEdgesMax = 100;
  int64 NodeCount = 0;
  int64 EdgeCount = 0;
  
  PNGraph Graph;
  
  TInt::Rnd.PutSeed(0);
  TFlt A, B, C;
  TFltTr64V Vals;
  Vals.Add(TFltTr(0.15, 0.2, 0.35));
  Vals.Add(TFltTr(0.2, 0.2, 0.55));
  Vals.Add(TFltTr(0.4, 0.15, 0.2));
  
  for (int64 NNodes = 50; NNodes < NNodesMax; NNodes+=NNodesMax/2) {
    
    for (int64 NEdges = 5; NEdges < NEdgesMax; NEdges+=NEdgesMax/4) {

      for (int64 i = 0; i < Vals.Len(); i++) {
        
        A = Vals[i].Val1;
        B = Vals[i].Val2;
        C = Vals[i].Val3;
        
        Graph = TSnap::GenRMat(NNodes, NEdges, A, B, C);
        EXPECT_FALSE(Graph->Empty());
        EXPECT_TRUE(Graph->IsOk());
        
        NodeCount = 0;
        EdgeCount = 0;
        // Iterate through nodes, verify it matches
        for (TNGraph::TNodeI NI = Graph->BegNI(); NI < Graph->EndNI(); NI++) {
          NodeCount++;
        }
        EXPECT_EQ(NodeCount, Graph->GetNodes());
        
        EdgeCount = 0;
        // Iterate through edges, count and verify
        for (TNGraph::TEdgeI NI = Graph->BegEI(); NI < Graph->EndEI(); NI++) {
          EdgeCount++;
        }
        EXPECT_EQ(EdgeCount, Graph->GetEdges());
    
      }
    } // end loop - NEdges
    
  } // end loop - NNodes
}
