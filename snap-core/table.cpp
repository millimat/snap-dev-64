void TPredicateNode::GetVariables(TStr64V& Variables) {
  if (Left != NULL) { Left->GetVariables(Variables); }
  if (Right != NULL) { Right->GetVariables(Variables); }
  if (Op == NOP) {
    if (Atom.Lvar != "" ) { Variables.Add(Atom.Lvar); }
    if (Atom.Rvar != "" ) { Variables.Add(Atom.Rvar); }
  }
}

void TPredicate::GetVariables(TStr64V& Variables) {
  Root->GetVariables(Variables);
}

TBool TPredicate::Eval() {
  TPredicateNode* Curr = Root;
  TPredicateNode* Prev = NULL;
  while (!(Curr == NULL && Prev == Root)) {
    // going down the tree
    if (Prev == NULL || Prev == Curr->Parent) {
      // left child exists and was not yet evaluated
      if (Curr->Left != NULL) {
        Prev = Curr;
        Curr = Curr->Left;
      } else if (Curr->Right != NULL) {
        Prev = Curr;
        Curr = Curr->Right;
      } else {
        Curr->Result = EvalAtomicPredicate(Curr->Atom);
        Prev = Curr;
        Curr = Curr->Parent;
      }
    } else if (Prev == Curr->Left) {
      // going back up through left (first) child
      switch (Curr->Op) {
        case NOT: {
          Assert(Curr->Right == NULL);
          Curr->Result = !(Prev->Result);
          Prev = Curr;
          Curr = Curr->Parent;
          break;
        }
        case AND: {
          Assert(Curr->Right != NULL);
          if (!Prev->Result) {
            Curr->Result = false;
            Prev = Curr;
            Curr = Curr->Parent;
          } else {
            Prev = Curr;
            Curr = Curr->Right;
          }
          break;
        }
        case OR: {
          Assert(Curr->Right != NULL);
          if (Prev->Result) {
            Curr->Result = true;
            Prev = Curr;
            Curr = Curr->Parent;
          } else {
            Prev = Curr;
            Curr = Curr->Right;
          }
          break;
        }
        case NOP: {
          break;
        }
      }
    } else {
      // going back up the tree from right (second) child
      Assert(Prev == Curr->Right);
      switch (Curr->Op) {
        case NOT: {
          Assert(Curr->Left == NULL);
          Curr->Result = !(Prev->Result);
          break;
        }
        case AND: {
          Assert(Curr->Left != NULL);
          Assert(Curr->Left->Result);
          Curr->Result = Prev->Result;
          break;
        }
        case OR: {
          Assert(Curr->Left != NULL);
          Assert(!Curr->Left->Result);
          Curr->Result = Prev->Result;
          break;
        }
        case NOP: {
          break;
        }
      }
      Prev = Curr;
      Curr = Curr->Parent;
    }
  }
  return Root->Result;
}

TBool TPredicate::EvalAtomicPredicate(const TAtomicPredicate& Atom) {
  switch (Atom.Type) {
    case atInt: {
      if (Atom.IsConst) {
        return EvalAtom<TInt64>(IntVars.GetDat(Atom.Lvar), Atom.IntConst, Atom.Compare);
      }
      return EvalAtom<TInt64>(IntVars.GetDat(Atom.Lvar), IntVars.GetDat(Atom.Rvar), Atom.Compare);
    }
    case atFlt: {
      if (Atom.IsConst) {
        return EvalAtom<TFlt>(FltVars.GetDat(Atom.Lvar), Atom.FltConst, Atom.Compare);
      }
      return EvalAtom<TFlt>(FltVars.GetDat(Atom.Lvar), FltVars.GetDat(Atom.Rvar), Atom.Compare);
    }
    case atStr: {
      if (Atom.IsConst) {
        return EvalAtom<TStr>(StrVars.GetDat(Atom.Lvar), Atom.StrConst, Atom.Compare);
      }
      return EvalAtom<TStr>(StrVars.GetDat(Atom.Lvar), StrVars.GetDat(Atom.Rvar), Atom.Compare);
    }
  }
  return false;
}

TInt64 const TTable::Last = -1;
TInt64 const TTable::Invalid = -2;

TInt64 TTable::UseMP = 1;

TRowIterator& TRowIterator::operator++(int) {
  return this->Next();
}

TRowIterator& TRowIterator::Next() {
  CurrRowIdx = Table->Next[CurrRowIdx];
  //Assert(CurrRowIdx != TTable::Invalid);
  return *this;
}

bool TRowIterator::operator < (const TRowIterator& RowI) const{
  if (CurrRowIdx == TTable::Last) { return false; }
  if (RowI.CurrRowIdx == TTable::Last) { return true; }
  return CurrRowIdx < RowI.CurrRowIdx;
}

bool TRowIterator::operator == (const TRowIterator& RowI) const {
  return CurrRowIdx == RowI.CurrRowIdx;
}

TInt64 TRowIterator::GetRowIdx() const {
  return CurrRowIdx;
}
// We do not check column type in the iterator.
TInt64 TRowIterator::GetIntAttr(TInt64 ColIdx) const {
  return Table->IntCols[ColIdx][CurrRowIdx];
}

TFlt TRowIterator::GetFltAttr(TInt64 ColIdx) const {
  return Table->FltCols[ColIdx][CurrRowIdx];
}

TStr TRowIterator::GetStrAttr(TInt64 ColIdx) const {
  return Table->GetStrVal(ColIdx, CurrRowIdx);
}

TInt64 TRowIterator::GetIntAttr(const TStr& Col) const {
  TInt64 ColIdx = Table->GetColIdx(Col);
  return Table->IntCols[ColIdx][CurrRowIdx];
}

TFlt TRowIterator::GetFltAttr(const TStr& Col) const {
  TInt64 ColIdx = Table->GetColIdx(Col);
  return Table->FltCols[ColIdx][CurrRowIdx];
}

TStr TRowIterator::GetStrAttr(const TStr& Col) const {
  return Table->GetStrVal(Col, CurrRowIdx);
}

TInt64 TRowIterator::GetStrMapByName(const TStr& Col) const {
  TInt64 ColIdx = Table->GetColIdx(Col);
  return Table->StrColMaps[ColIdx][CurrRowIdx];
}

TInt64 TRowIterator::GetStrMapById(TInt64 ColIdx) const {
  return Table->StrColMaps[ColIdx][CurrRowIdx];
}

TBool TRowIterator::CompareAtomicConst(TInt64 ColIdx, const TPrimitive& Val, TPredComp Cmp) {
  TBool Result;
  switch (Val.GetType()) {
    case atInt:
      Result = TPredicate::EvalAtom(GetIntAttr(ColIdx), Val.GetInt(), Cmp);
      break;
    case atFlt:
      Result = TPredicate::EvalAtom(GetFltAttr(ColIdx), Val.GetFlt(), Cmp);
      break;
    case atStr:
      Result = TPredicate::EvalStrAtom(GetStrAttr(ColIdx), Val.GetStr(), Cmp);
      break;
    default:
      Result = TBool(false);
  }
  return Result;
}

TBool TRowIterator::CompareAtomicConstTStr(TInt64 ColIdx, const TStr& Val, TPredComp Cmp) {
  TBool Result;
  //printf("string compare\n");
  Result = TPredicate::EvalStrAtom(GetStrAttr(ColIdx), Val, Cmp);
  return Result;
}

TRowIteratorWithRemove::TRowIteratorWithRemove(TInt64 RowIdx, TTable* TablePtr) :
  CurrRowIdx(RowIdx), Table(TablePtr), Start(RowIdx == TablePtr->FirstValidRow) {}

TRowIteratorWithRemove& TRowIteratorWithRemove::operator++(int) {
  return this->Next();
}

TRowIteratorWithRemove& TRowIteratorWithRemove::Next() {
  CurrRowIdx = GetNextRowIdx();
  Start = false;
  Assert(CurrRowIdx != TTable::Invalid);
  return *this;
}

bool TRowIteratorWithRemove::operator < (const TRowIteratorWithRemove& RowI) const {
  if (CurrRowIdx == TTable::Last) { return false; }
  if (RowI.CurrRowIdx == TTable::Last) { return true; }
  return CurrRowIdx < RowI.CurrRowIdx;
}

bool TRowIteratorWithRemove::operator == (const TRowIteratorWithRemove& RowI) const {
  return CurrRowIdx == RowI.CurrRowIdx;
}

TInt64 TRowIteratorWithRemove::GetRowIdx() const {
  return CurrRowIdx;
}

TInt64 TRowIteratorWithRemove::GetNextRowIdx() const {
  return (Start ? Table->FirstValidRow : Table->Next[CurrRowIdx]);
}

// We do not check column type in the iterator.
TInt64 TRowIteratorWithRemove::GetNextIntAttr(TInt64 ColIdx) const {
  return Table->IntCols[ColIdx][GetNextRowIdx()];
}

TFlt TRowIteratorWithRemove::GetNextFltAttr(TInt64 ColIdx) const {
  return Table->FltCols[ColIdx][GetNextRowIdx()];
}

TStr TRowIteratorWithRemove::GetNextStrAttr(TInt64 ColIdx) const {
  return Table->GetStrVal(ColIdx, GetNextRowIdx());
}

TInt64 TRowIteratorWithRemove::GetNextIntAttr(const TStr& Col) const {
  TInt64 ColIdx = Table->GetColIdx(Col);
  return Table->IntCols[ColIdx][GetNextRowIdx()];
}

TFlt TRowIteratorWithRemove::GetNextFltAttr(const TStr& Col) const {
  TInt64 ColIdx = Table->GetColIdx(Col);
  return Table->FltCols[ColIdx][GetNextRowIdx()];
}

TStr TRowIteratorWithRemove::GetNextStrAttr(const TStr& Col) const {
  return Table->GetStrVal(Col, GetNextRowIdx());
}

TBool TRowIteratorWithRemove::IsFirst() const {
  return CurrRowIdx == Table->FirstValidRow;
}

void TRowIteratorWithRemove::RemoveNext() {
  Table->RemoveRow(GetNextRowIdx(), CurrRowIdx);
}

TBool TRowIteratorWithRemove::CompareAtomicConst(TInt64 ColIdx, const TPrimitive& Val, TPredComp Cmp) {
  TBool Result;
  switch (Val.GetType()) {
    case atInt:
      Result = TPredicate::EvalAtom(GetNextIntAttr(ColIdx), Val.GetInt(), Cmp);
      break;
    case atFlt:
      Result = TPredicate::EvalAtom(GetNextFltAttr(ColIdx), Val.GetFlt(), Cmp);
      break;
    case atStr:
      Result = TPredicate::EvalStrAtom(GetNextStrAttr(ColIdx), Val.GetStr(), Cmp);
      break;
    default:
      Result = TBool(false);
  }
  return Result;
}

// Better not use default constructor as it leads to a memory leak.
// - OR - implement a destructor.
TTable::TTable(): Context(new TTableContext), NumRows(0), NumValidRows(0),
  FirstValidRow(0), LastValidRow(-1) {}

TTable::TTable(TTableContext* Context): Context(Context), NumRows(0),
  NumValidRows(0), FirstValidRow(0), LastValidRow(-1) {}

TTable::TTable(const Schema& TableSchema, TTableContext* Context): Context(Context),
  NumRows(0), NumValidRows(0), FirstValidRow(0), LastValidRow(-1), IsNextDirty(0) {
  TInt64 IntColCnt = 0;
  TInt64 FltColCnt = 0;
  TInt64 StrColCnt = 0;
  for (TInt64 i = 0; i < TableSchema.Len(); i++) {
    TStr ColName = TableSchema[i].Val1;
    TAttrType ColType = TableSchema[i].Val2;
    AddSchemaCol(ColName, ColType);
    switch (ColType) {
      case atInt:
        AddColType(ColName, atInt, IntColCnt);
        IntColCnt++;
        break;
      case atFlt:
        AddColType(ColName, atFlt, FltColCnt);
        FltColCnt++;
        break;
      case atStr:
        AddColType(ColName, atStr, StrColCnt);
        StrColCnt++;
        break;
    }
  }
  IntCols = TVec<TInt64V, int64>(IntColCnt);
  FltCols = TVec<TFlt64V, int64>(FltColCnt);
  StrColMaps = TVec<TInt64V, int64>(StrColCnt);
}

void TTable::GenerateColTypeMap(THash<TStr,TPair<TInt64,TInt64>, int64 > & ColTypeIntMap) {
  ColTypeMap.Clr();
  Sch.Clr();
  for (THash<TStr,TPair<TInt64,TInt64>, int64 >::TIter it = ColTypeIntMap.BegI(); it < ColTypeIntMap.EndI(); it++) {
    TPair<TInt64,TInt64> dat = it.GetDat();
    switch (dat.GetVal1()) {
      case 0:
        AddColType(it.GetKey(), atInt, dat.GetVal2());
        AddSchemaCol(it.GetKey(), atInt);
        break;
      case 1:
        AddColType(it.GetKey(), atFlt, dat.GetVal2());
        AddSchemaCol(it.GetKey(), atFlt);
        break;
      case 2:
        AddColType(it.GetKey(), atStr, dat.GetVal2());
        AddSchemaCol(it.GetKey(), atStr);
        break;
    }
  }
  IsNextDirty = 0;
}

void TTable::LoadTableShM(TShMIn& ShMIn, TTableContext* ContextTable) {
  Context = ContextTable;
  NumRows = TInt64(ShMIn);
  NumValidRows = TInt64(ShMIn);
  FirstValidRow = TInt64(ShMIn);
  LastValidRow = TInt64(ShMIn);
  Next.LoadShM(ShMIn);

  TLoadVecInit Fn;
  IntCols.LoadShM(ShMIn, Fn);
  FltCols.Load(ShMIn);
  StrColMaps.LoadShM(ShMIn, Fn);
  THash<TStr,TPair<TInt64,TInt64>, int64 > ColTypeIntMap;
  ColTypeIntMap.LoadShM(ShMIn);

  GenerateColTypeMap(ColTypeIntMap);
}

TTable::TTable(TSIn& SIn, TTableContext* Context): Context(Context), NumRows(SIn),
  NumValidRows(SIn), FirstValidRow(SIn), LastValidRow(SIn), Next(SIn), IntCols(SIn),
  FltCols(SIn), StrColMaps(SIn) {
  THash<TStr,TPair<TInt64,TInt64>, int64 > ColTypeIntMap(SIn);
  GenerateColTypeMap(ColTypeIntMap);
}

TTable::TTable(const TIntInt64H& H, const TStr& Col1, const TStr& Col2,
 TTableContext* Context, const TBool IsStrKeys) : Context(Context), NumRows(H.Len()),
  NumValidRows(H.Len()), FirstValidRow(0), LastValidRow(H.Len()-1) {
    TAttrType KeyType = IsStrKeys ? atStr : atInt;
    AddSchemaCol(Col1, KeyType);
    AddSchemaCol(Col2, atInt);
    AddColType(Col1, KeyType, 0);
    AddColType(Col2, atInt, 1);
    if (IsStrKeys) {
      StrColMaps = TVec<TInt64V, int64>(1);
      IntCols = TVec<TInt64V, int64>(1);
      H.GetKeyV(StrColMaps[0]);
      H.GetDatV(IntCols[0]);
    } else {
      IntCols = TVec<TInt64V, int64>(2);
      H.GetKeyV(IntCols[0]);
      H.GetDatV(IntCols[1]);
    }
    Next = TInt64V(NumRows);
    for (TInt64 i = 0; i < NumRows; i++) {
      Next[i] = i+1;
    }
    Next[NumRows-1] = Last;
    IsNextDirty = 0;
    InitIds();
}

TTable::TTable(const TIntFlt64H& H, const TStr& Col1, const TStr& Col2,
 TTableContext* Context, const TBool IsStrKeys) : Context(Context),
  NumRows(H.Len()), NumValidRows(H.Len()), FirstValidRow(0), LastValidRow(H.Len()-1) {
  TAttrType KeyType = IsStrKeys ? atStr : atInt;
  AddSchemaCol(Col1, KeyType);
  AddSchemaCol(Col2, atFlt);
  AddColType(Col1, KeyType, 0);
  AddColType(Col2, atFlt, 0);
  if (IsStrKeys) {
    StrColMaps = TVec<TInt64V, int64>(1);
    H.GetKeyV(StrColMaps[0]);
  } else {
    IntCols = TVec<TInt64V, int64>(1);
    H.GetKeyV(IntCols[0]);
  }
  FltCols = TVec<TFlt64V, int64>(1);
  H.GetDatV(FltCols[0]);
  Next = TInt64V(NumRows);
  for (TInt64 i = 0; i < NumRows; i++) {
    Next[i] = i+1;
  }
  Next[NumRows-1] = Last;
  IsNextDirty = 0;
  InitIds();
}

TTable::TTable(const TTable& Table, const TInt64V& RowIDs) : Context(Table.Context),
  Sch(Table.Sch), SrcCol(Table.SrcCol), DstCol(Table.DstCol), EdgeAttrV(Table.EdgeAttrV),
  SrcNodeAttrV(Table.SrcNodeAttrV), DstNodeAttrV(Table.DstNodeAttrV),
  CommonNodeAttrs(Table.CommonNodeAttrs) {
  ColTypeMap = Table.ColTypeMap;
  IntCols = TVec<TInt64V, int64>(Table.IntCols.Len());
  FltCols = TVec<TFlt64V, int64>(Table.FltCols.Len());
  StrColMaps = TVec<TInt64V, int64>(Table.StrColMaps.Len());
  FirstValidRow = 0;
  LastValidRow = -1;
  NumRows = 0;
  NumValidRows = 0;
  AddSelectedRows(Table, RowIDs);
  IsNextDirty = 0;
  InitIds();
}

void TTable::GetSchema(const TStr& InFNm, Schema& S, const char& Separator) {
  // Determine Attr Type
  // Assume that the data is tab separated
  TSsParser Ss(InFNm, '\t', false, false, false);
  TInt64 rowsToPeek = 1000;
  TInt64 currRow = 0;
  TInt64 lastComment = 0;
  while (Ss.Next()) {
    if (Ss.IsCmt()) {
      lastComment += 1;
    }
    else break;
  }
  if (Ss.Eof()) {TExcept::Throw("No Data to determine attribute types!");}
  TInt64 numCols = Ss.GetFlds();
  TVec<TAttrType, int64> colAttrV(numCols);
  colAttrV.PutAll(atInt);
  while (true) {
    for (TInt64 i = 0; i < numCols; i++) {
      if (Ss.IsInt(i)) {
      }
      else if (Ss.IsFlt(i)) {
        colAttrV[i] = atFlt;
      }
      else {
        colAttrV[i] = atStr;
      }
    }
    currRow++;
    if (currRow > rowsToPeek || Ss.Eof()) break;
    Ss.Next();
  }
  // Default Separator is tab
  TSsParser SsNames(InFNm, Separator, false, false, false);
  for (int64 i = 0; i < lastComment; i++) { SsNames.Next();}
  TVec<TStr, int64> attrV;
  TStr first(SsNames[0]);
  int64 begin = 0;
  TStr comment('#');
  if (first != comment) {
    for (int64 i = 1; i < first.Len(); i++){
      if (first[i] != ' ') { begin = i; break;}
    }
    attrV.Add(first.GetSubStr(begin));
  }
  for (int64 i = 1; i < SsNames.GetFlds(); i++) {attrV.Add(SsNames[i]);}
  for (TInt64 i = 0; i < numCols; i++) {
    S.Add(TPair<TStr,TAttrType>(attrV[i],colAttrV[i]));
  }
}

#ifdef GCC_ATOMIC
void TTable::LoadSSPar(PTable& T, const Schema& S, const TStr& InFNm, const TInt64V& RelevantCols,
                        const char& Separator, TBool HasTitleLine) {
  // preloaded necessary variables
  TInt64 RowLen = T->Sch.Len();
  TVec<TAttrType, int64> ColTypes = TVec<TAttrType, int64>(RowLen);
  for (TInt64 i = 0; i < RowLen; i++) {
    ColTypes[i] = T->GetSchemaColType(i);
  }

  TSsParserMP Ss(InFNm, Separator);
  Ss.SkipCommentLines();

  // if title line (i.e. names of the columns) is included as first row in the
  // input file - use it to validate schema
  if (HasTitleLine) {
    Ss.Next();
    if (S.Len() != Ss.GetFlds()) {
      printf("%s\n", Ss[0]); TExcept::Throw("Table Schema Mismatch!");
    }
    for (TInt64 i = 0; i < Ss.GetFlds(); i++) {
      // remove carriage return char
      int64 L = strlen(Ss[i]);
      if (Ss[i][L-1] < ' ') { Ss[i][L-1] = 0; }
      if (NormalizeColName(S[i].Val1) != NormalizeColName(Ss[i])) { TExcept::Throw("Table Schema Mismatch!"); }
    }
  }

  // Divide remaining part of stream into equal sized chunks
  // Find starting position in stream for each thread
  int64 Cnt = 0;
  uint64 Pos = Ss.GetStreamPos();
  uint64 Len = Ss.GetStreamLen();
  uint64 Rem = Len - Pos;
  int64 NumThreads = omp_get_max_threads();

  uint64 Delta = Rem / NumThreads;
  if (Delta < 1) Delta = 1;

  TVec<uint64, int64> StartIntV(NumThreads);
  TVec<uint64, int64> LineCountV(NumThreads);
  TVec<uint64, int64> PrefixSumV(NumThreads);

  StartIntV[0] = Pos;
  for (int64 i = 1; i < NumThreads; i++) {
    StartIntV[i] = StartIntV[i-1] + Delta;
  }
  StartIntV.Add(Len);

  // Find number of lines handled by each thread
  omp_set_num_threads(NumThreads);
  #pragma omp parallel for schedule(dynamic) reduction(+:Cnt)
  for (int64 i = 0; i < NumThreads; i++) {
    LineCountV[i] = Ss.CountNewLinesInRange(StartIntV[i], StartIntV[i+1]);
    Cnt += LineCountV[i];
  }

  // Calculate row index offsets for each thread
  PrefixSumV[0] = 0;
  for (int64 i = 1; i < NumThreads; i++) {
    PrefixSumV[i] = PrefixSumV[i-1] + LineCountV[i-1];
  }
  Ss.SetStreamPos(Pos);

  // allocate memory for columns
  TInt64 IntColIdx = 0;
  TInt64 FltColIdx = 0;
  for (TInt64 i = 0; i < RowLen; i++) {
    switch (ColTypes[i]) {
      case atInt:
        T->IntCols[IntColIdx].Gen(Cnt);
        IntColIdx++;
        break;
      case atFlt:
        T->FltCols[FltColIdx].Gen(Cnt);
        FltColIdx++;
        break;
      case atStr:
        break;
    }
  }

  Cnt = 0;
  omp_set_num_threads(NumThreads);
  #pragma omp parallel for schedule(dynamic) reduction(+:Cnt)
  for (int64 i = 0; i < NumThreads; i++) {
    // calculate beginning of each line handled by thread
    TVec<uint64> LineStartPosV = Ss.GetStartPosV(StartIntV[i], StartIntV[i+1]);
    // parse line and fill rows
    for (uint64 k = 0; k < (uint64) LineStartPosV.Len(); k++) {
      TVec<char*> FieldsV;
      Ss.NextFromIndex(LineStartPosV[k], FieldsV);
      if (FieldsV.Len() != S.Len()) {
        TExcept::Throw("Error reading tsv file");
      }
      TInt64 IntColIdx = 0;
      TInt64 FltColIdx = 0;
      TInt64 RowIdx = PrefixSumV[i] + k;

      for (TInt64 j = 0; j < RowLen; j++) {
        switch (ColTypes[j]) {
          case atInt:
            if (RelevantCols.Len() == 0) {
              T->IntCols[IntColIdx][RowIdx] = \
                (Ss.GetIntFromFldV(FieldsV, j));
            } else {
              T->IntCols[IntColIdx][RowIdx] = \
                (Ss.GetIntFromFldV(FieldsV, RelevantCols[j]));
            }
            IntColIdx++;
            break;
          case atFlt:
            if (RelevantCols.Len() == 0) {
              T->FltCols[FltColIdx][RowIdx] = \
                (Ss.GetFltFromFldV(FieldsV, j));
            } else {
              T->FltCols[FltColIdx][RowIdx] = \
                (Ss.GetFltFromFldV(FieldsV, RelevantCols[j]));
            }
            FltColIdx++;
            break;
          case atStr:
            TExcept::Throw("TTable::LoadSS:: Str Col found\n");
            break;
        }
      }
      Cnt++;
    }
  }

  // set number of rows and "Next" vector
  T->NumRows = Cnt;
  T->NumValidRows = T->NumRows;

  T->Next.Clr();
  T->Next.Gen(Cnt);

  omp_set_num_threads(NumThreads);
  #pragma omp parallel for schedule(dynamic, 10000)
  for (int64 i = 0; i < Cnt-1; i++) {
    T->Next[i] = i+1;
  }
  T->IsNextDirty = 0;
  T->Next[Cnt-1] = Last;
  T->LastValidRow = T->NumRows - 1;

  T->IdColName = "_id";
  TInt64 IdCol = T->IntCols.Add();
  T->IntCols[IdCol].Gen(Cnt);

  // initialize ID column
  omp_set_num_threads(NumThreads);
  #pragma omp parallel for schedule(dynamic, 10000)
  for (int64 i = 0; i < Cnt; i++) {
    T->IntCols[IdCol][i] = i;
  }

  T->AddSchemaCol(T->IdColName, atInt);
  T->AddColType(T->IdColName, atInt, T->IntCols.Len()-1);
}
#endif // GCC_ATOMIC

void TTable::LoadSSSeq(
 PTable& T, const Schema& S, const TStr& InFNm, const TInt64V& RelevantCols,
 const char& Separator, TBool HasTitleLine) {
  // preloaded necessary variables
  int64 RowLen = T->Sch.Len();
  TVec<TAttrType, int64> ColTypes = TVec<TAttrType, int64>(RowLen);
  for (int64 i = 0; i < RowLen; i++) {
    ColTypes[i] = T->GetSchemaColType(i);
  }

  // Sequential load
  TSsParser Ss(InFNm, Separator);
  // if title line (i.e. names of the columns) is included as first row in the
  // input file - use it to validate schema
  if (HasTitleLine) {
    Ss.Next();
    if (S.Len() != Ss.GetFlds()) {
      printf("%s\n", Ss[0]); TExcept::Throw("Table Schema Mismatch!");
    }
    for (int64 i = 0; i < Ss.GetFlds(); i++) {
      // remove carriage return char
      int64 L = strlen(Ss[i]);
      if (Ss[i][L-1] < ' ') { Ss[i][L-1] = 0; }
      if (NormalizeColName(S[i].Val1) != NormalizeColName(Ss[i])) { TExcept::Throw("Table Schema Mismatch!"); }
    }
  }

  // populate table columns
  //printf("starting to populate table\n");
  uint64 Cnt = 0;
  while (Ss.Next()) {
    int64 IntColIdx = 0;
    int64 FltColIdx = 0;
    int64 StrColIdx = 0;
    Assert(Ss.GetFlds() == S.Len()); // compiled only in debug
    if (Ss.GetFlds() != S.Len()) {
      printf("%s\n", Ss[S.Len()]); TExcept::Throw("Error reading tsv file");
    }
    for (int64 i = 0; i < RowLen; i++) {
      switch (ColTypes[i]) {
        case atInt:
          if (RelevantCols.Len() == 0) {
            T->IntCols[IntColIdx].Add(Ss.GetInt64(i));
          } else {
            T->IntCols[IntColIdx].Add(Ss.GetInt64(RelevantCols[i]));
          }
          IntColIdx++;
          break;
        case atFlt:
          if (RelevantCols.Len() == 0) {
            T->FltCols[FltColIdx].Add(Ss.GetFlt(i));
          } else {
            T->FltCols[FltColIdx].Add(Ss.GetFlt(RelevantCols[i]));
          }
          FltColIdx++;
          break;
        case atStr:
          int64 ColIdx;
          if (RelevantCols.Len() == 0) {
            ColIdx = i;
          } else {
            ColIdx = RelevantCols[i];
          }
          TStr Sval = TStr(Ss[ColIdx]);
          T->AddStrVal(StrColIdx, Sval);
          StrColIdx++;
          break;
      }
    }
    Cnt += 1;
  }
  //printf("finished populating table\n");
  // set number of rows and "Next" vector
  T->NumRows = static_cast<int64>(Cnt);
  T->NumValidRows = T->NumRows;

  T->Next.Clr();
  T->Next.Gen(static_cast<int64>(Cnt));
  for (uint64 i = 0; i < Cnt-1; i++) {
    T->Next[static_cast<int64>(i)] = static_cast<int64>(i+1);
  }
  T->IsNextDirty = 0;
  T->Next[static_cast<int64>(Cnt-1)] = Last;
  T->LastValidRow = T->NumRows - 1;

  T->InitIds();
}

PTable TTable::LoadSS(const Schema& S, const TStr& InFNm, TTableContext* Context,
 const TInt64V& RelevantCols, const char& Separator, TBool HasTitleLine) {
  TVec<uint64, int64> IntGroupByCols;
  bool NoStringCols = true;

  // find the schema for the new table which contains only relevant columns
  Schema SR;
  if (RelevantCols.Len() == 0) {
    SR = S;
  } else {
    for (int64 i = 0; i < RelevantCols.Len(); i++) {
      SR.Add(S[RelevantCols[i]]);
    }
  }
  PTable T = New(SR, Context);

  // find col types and check for string cols
  for (int64 i = 0; i < SR.Len(); i++) {
    if (T->GetSchemaColType(i) == atStr) {
      NoStringCols = false;
      break;
    }
  }

  if (GetMP() && NoStringCols) {
    // Right now, can load in parallel only in Linux (for mmap) and if
    // there are no string columns
#ifdef GLib_LINUX
     LoadSSPar(T, S, InFNm, RelevantCols, Separator, HasTitleLine);
#else
    LoadSSSeq(T, S, InFNm, RelevantCols, Separator, HasTitleLine);
#endif
  } else {
    LoadSSSeq(T, S, InFNm, RelevantCols, Separator, HasTitleLine);
  }
  return T;
}

PTable TTable::LoadSS(const Schema& S, const TStr& InFNm, TTableContext* Context,
 const char& Separator, TBool HasTitleLine) {
  return LoadSS(S, InFNm, Context, TInt64V(), Separator, HasTitleLine);
}

void TTable::SaveSS(const TStr& OutFNm) {
  if (NumValidRows == 0) {
    printf("Table is empty");
    return;
  }
  FILE* F = fopen(OutFNm.CStr(), "w");
  // debug
  if (F == NULL) {
    printf("failed to open file %s\n", OutFNm.CStr());
    perror("fail ");
    return;
  }

  Dump(F);

#if 0
  Schema DSch = DenormalizeSchema();

  TInt L = Sch.Len();
  // print title (schema)
  fprintf(F, "# ");
  for (TInt i = 0; i < L-1; i++) {
    fprintf(F, "%s\t", DSch[i].Val1.CStr());
  }
  fprintf(F, "%s\n", DSch[L-1].Val1.CStr());
  // print table contents
  for (TRowIterator RowI = BegRI(); RowI < EndRI(); RowI++) {
    for (TInt i = 0; i < L; i++) {
      char C = (i == L-1) ? '\n' : '\t';
      switch (GetSchemaColType(i)) {
        case atInt: {
          fprintf(F, "%d%c", RowI.GetIntAttr(GetSchemaColName(i)).Val, C);
          break;
        }
        case atFlt: {
          fprintf(F, "%f%c", RowI.GetFltAttr(GetSchemaColName(i)).Val, C);
          break;
        }
        case atStr: {
          fprintf(F, "%s%c", RowI.GetStrAttr(GetSchemaColName(i)).CStr(), C);
          break;
        }
      }
    }
  }
#endif
  fclose(F);
}

void TTable::SaveBin(const TStr& OutFNm) {
  TFOut SOut(OutFNm);
  Save(SOut);
}

void TTable::Save(TSOut& SOut) {
  NumRows.Save(SOut);
  NumValidRows.Save(SOut);
  FirstValidRow.Save(SOut);
  LastValidRow.Save(SOut);
  Next.Save(SOut);
  IntCols.Save(SOut);
  FltCols.Save(SOut);
  StrColMaps.Save(SOut);

  THash<TStr,TPair<TInt64,TInt64>, int64 > ColTypeIntMap;
  TInt64 atIntVal = TInt64(0);
  TInt64 atFltVal = TInt64(1);
  TInt64 atStrVal = TInt64(2);
  for (THash<TStr,TPair<TAttrType,TInt64>, int64 >::TIter it = ColTypeMap.BegI(); it < ColTypeMap.EndI(); it++) {
    TPair<TAttrType,TInt64> dat = it.GetDat();
    TStr DColName = DenormalizeColName(it.GetKey());
    switch (dat.GetVal1()) {
      case atInt:
        ColTypeIntMap.AddDat(DColName, TPair<TInt64,TInt64>(atIntVal, dat.GetVal2()));
        break;
      case atFlt:
        ColTypeIntMap.AddDat(DColName, TPair<TInt64,TInt64>(atFltVal, dat.GetVal2()));
        break;
      case atStr:
        ColTypeIntMap.AddDat(DColName, TPair<TInt64,TInt64>(atStrVal, dat.GetVal2()));
        break;
    }
  }
  ColTypeIntMap.Save(SOut);
  SOut.Flush();
}

void TTable::Dump(FILE *OutF) const {
  TInt64 L = Sch.Len();
  Schema DSch = DenormalizeSchema();

  // LoadSS() will not throw away lines with #
  //fprintf(OutF, "# Table: rows: %d, columns: %d\n", GetNumValidRows(), GetNodes());
  // print title (schema), LoadSS() will take first line as (optional) schema
  fprintf(OutF, "# ");
  for (TInt64 i = 0; i < L-1; i++) {
    fprintf(OutF, "%s\t", DSch[i].Val1.CStr());
  }
  fprintf(OutF, "%s\n", DSch[L-1].Val1.CStr());
  // print table contents
  for (TRowIterator RowI = BegRI(); RowI < EndRI(); RowI++) {
    for (TInt64 i = 0; i < L; i++) {
      char C = (i == L-1) ? '\n' : '\t';
      switch (GetSchemaColType(i)) {
        case atInt: {
          fprintf(OutF, "%s%c", TInt64::GetStr(RowI.GetIntAttr(GetSchemaColName(i)).Val).GetCStr(), C);
          break;
        }
        case atFlt: {
          fprintf(OutF, "%f%c", RowI.GetFltAttr(GetSchemaColName(i)).Val, C);
          break;
        }
        case atStr: {
          fprintf(OutF, "%s%c", RowI.GetStrAttr(GetSchemaColName(i)).CStr(), C);
          break;
        }
      }
    }
  }
}

TTableContext* TTable::ChangeContext(TTableContext* NewContext) {
  TInt64 L = Sch.Len();

#if 0
  // print table on the input, iterate over all columns
  for (TInt i = 0; i < L; i++) {
    // skip non-string columns
    if (GetSchemaColType(i) != atStr) {
      continue;
    }

    TInt ColIdx = GetColIdx(GetSchemaColName(i));

    // iterate over all rows
    for (TRowIterator RowI = BegRI(); RowI < EndRI(); RowI++) {
      TInt RowIdx = RowI.GetRowIdx();
      TInt KeyId = StrColMaps[ColIdx][RowIdx];
      printf("ChangeContext in  %d  %d  %d  .%s.\n",
          ColIdx.Val, RowIdx.Val, KeyId.Val, GetStrVal(ColIdx, RowIdx).CStr());
    }
  }
#endif

  // add strings to the new context, change values
  // iterate over all columns
  for (TInt64 i = 0; i < L; i++) {
    // skip non-string columns
    if (GetSchemaColType(i) != atStr) {
      continue;
    }

    TInt64 ColIdx = GetColIdx(GetSchemaColName(i));

    // iterate over all rows
    for (TRowIterator RowI = BegRI(); RowI < EndRI(); RowI++) {
      TInt64 RowIdx = RowI.GetRowIdx();
      // get the string
      TStr Key = GetStrVal(ColIdx, RowIdx);
      // add the string to the new context
      TInt64 KeyId = TInt64(NewContext->StringVals.AddKey(Key));
      // change the value in the table
      StrColMaps[ColIdx][RowIdx] = KeyId;
    }
  }

  // set the new context
  Context = NewContext;
  return Context;
}

void TTable::AddStrVal(const TInt64& ColIdx, const TStr& Key) {
  TInt64 KeyId = TInt64(Context->StringVals.AddKey(Key));
  //printf("TTable::AddStrVal2  %d  .%s.  %d\n", ColIdx.Val, Key.CStr(), KeyId.Val);
  StrColMaps[ColIdx].Add(KeyId);
}

void TTable::AddStrVal(const TStr& Col, const TStr& Key) {
  if (GetColType(Col) != atStr) {
    TExcept::Throw(Col + " is not a string valued column");
  }
  //printf("TTable::AddStrVal1  .%s.  .%s.\n", Col.CStr(), Key.CStr());
  AddStrVal(GetColIdx(Col), Key);
}

void TTable::AddGraphAttribute(const TStr& Attr, TBool IsEdge, TBool IsSrc, TBool IsDst) {
  if (!IsColName(Attr)) { TExcept::Throw(Attr + ": No such column"); }
  if (IsEdge) { EdgeAttrV.Add(NormalizeColName(Attr)); }
  if (IsSrc) { SrcNodeAttrV.Add(NormalizeColName(Attr)); }
  if (IsDst) { DstNodeAttrV.Add(NormalizeColName(Attr)); }
}

void TTable::AddGraphAttributeV(TStr64V& Attrs, TBool IsEdge, TBool IsSrc, TBool IsDst) {
  for (TInt64 i = 0; i < Attrs.Len(); i++) {
    if (!IsColName(Attrs[i])) {
      TExcept::Throw(Attrs[i] + ": no such column");
    }
  }
  for (TInt64 i = 0; i < Attrs.Len(); i++) {
    if (IsEdge) { EdgeAttrV.Add(NormalizeColName(Attrs[i])); }
    if (IsSrc) { SrcNodeAttrV.Add(NormalizeColName(Attrs[i])); }
    if (IsDst) { DstNodeAttrV.Add(NormalizeColName(Attrs[i])); }
  }
}

TStr64V TTable::GetSrcNodeIntAttrV() const {
  TStr64V IntNA = TStr64V(IntCols.Len(),0);
  for (TInt64 i = 0; i < SrcNodeAttrV.Len(); i++) {
    TStr Attr = SrcNodeAttrV[i];
    if (GetColType(Attr) == atInt) {
      IntNA.Add(Attr);
    }
  }
  return IntNA;
}

TStr64V TTable::GetDstNodeIntAttrV() const {
  TStr64V IntNA = TStr64V(IntCols.Len(),0);
  for (TInt64 i = 0; i < DstNodeAttrV.Len(); i++) {
    TStr Attr = DstNodeAttrV[i];
    if (GetColType(Attr) == atInt) {
      IntNA.Add(Attr);
    }
  }
  return IntNA;
}

TStr64V TTable::GetEdgeIntAttrV() const {
  TStr64V IntEA = TStr64V(IntCols.Len(),0);
  for (TInt64 i = 0; i < EdgeAttrV.Len(); i++) {
    TStr Attr = EdgeAttrV[i];
    if (GetColType(Attr) == atInt) {
      IntEA.Add(Attr);
    }
  }
  return IntEA;
}

TStr64V TTable::GetSrcNodeFltAttrV() const {
  TStr64V FltNA = TStr64V(FltCols.Len(),0);
  for (TInt64 i = 0; i < SrcNodeAttrV.Len(); i++) {
    TStr Attr = SrcNodeAttrV[i];
    if (GetColType(Attr) == atFlt) {
      FltNA.Add(Attr);
    }
  }
  return FltNA;
}

TStr64V TTable::GetDstNodeFltAttrV() const {
  TStr64V FltNA = TStr64V(FltCols.Len(),0);
  for (TInt64 i = 0; i < DstNodeAttrV.Len(); i++) {
    TStr Attr = DstNodeAttrV[i];
    if (GetColType(Attr) == atFlt) {
      FltNA.Add(Attr);
    }
  }
  return FltNA;
}

TStr64V TTable::GetEdgeFltAttrV() const {
  TStr64V FltEA = TStr64V(FltCols.Len(),0);;
  for (TInt64 i = 0; i < EdgeAttrV.Len(); i++) {
    TStr Attr = EdgeAttrV[i];
    if (GetColType(Attr) == atFlt) {
      FltEA.Add(Attr);
    }
  }
  return FltEA;
}

TStr64V TTable::GetSrcNodeStrAttrV() const {
  TStr64V StrNA = TStr64V(StrColMaps.Len(),0);
  for (TInt64 i = 0; i < SrcNodeAttrV.Len(); i++) {
    TStr Attr = SrcNodeAttrV[i];
    if (GetColType(Attr) == atStr) {
      StrNA.Add(Attr);
    }
  }
  return StrNA;
}

TStr64V TTable::GetDstNodeStrAttrV() const {
  TStr64V StrNA = TStr64V(StrColMaps.Len(),0);
  for (TInt64 i = 0; i < DstNodeAttrV.Len(); i++) {
    TStr Attr = DstNodeAttrV[i];
    if (GetColType(Attr) == atStr) {
      StrNA.Add(Attr);
    }
  }
  return StrNA;
}


TStr64V TTable::GetEdgeStrAttrV() const {
  TStr64V StrEA = TStr64V(StrColMaps.Len(),0);
  for (TInt64 i = 0; i < EdgeAttrV.Len(); i++) {
    TStr Attr = EdgeAttrV[i];
    if (GetColType(Attr) == atStr) {
      StrEA.Add(Attr);
    }
  }
  return StrEA;
}

void TTable::Rename(const TStr& column, const TStr& NewLabel) {
  // This function is necessary, for example to take the union of two tables
  // where the attribute names don't match.
  if (!IsColName(column)) { TExcept::Throw("no such column " + column); }
  TPair<TAttrType,TInt64> ColVal = GetColTypeMap(column);
  DelColType(column);
  AddColType(NewLabel, ColVal);
  TStr NColName = NormalizeColName(column);
  TStr NLabel = NormalizeColName(NewLabel);
  for (TInt64 c = 0; c < Sch.Len(); c++) {
    if (Sch[c].Val1 == NColName) {
      Sch.SetVal(c, TPair<TStr, TAttrType>(NLabel, Sch[c].Val2));
      break;
    }
  }
}

void TTable::RemoveFirstRow() {
  if (FirstValidRow == LastValidRow) {
    LastValidRow = -1;
  }

  TInt64 Old = FirstValidRow;
  FirstValidRow = Next[FirstValidRow];
  Next[Old] = TTable::Invalid;
  NumValidRows--;
  TInt64 IdColIdx = GetColIdx(GetIdColName());
  if (IdColIdx != -1) {
    RowIdMap.AddDat(IntCols[IdColIdx][Old], Invalid);
  }
}

void TTable::RemoveRow(TInt64 RowIdx, TInt64 PrevRowIdx) {
  if (RowIdx == FirstValidRow) {
    RemoveFirstRow();
    return;
  }
  Assert(RowIdx != TTable::Invalid);
  if (RowIdx == TTable::Last) { return; }
  Next[PrevRowIdx] = Next[RowIdx];
  if (LastValidRow == RowIdx) {
    LastValidRow = RowIdx;
  }
  Next[RowIdx] = TTable::Invalid;
  NumValidRows--;
  TInt64 IdColIdx = GetColIdx(GetIdColName());
  if (IdColIdx != -1) {
    RowIdMap.AddDat(IntCols[IdColIdx][RowIdx], Invalid);
  }
}

void TTable::KeepSortedRows(const TInt64V& KeepV) {
  TIntInt64H KeepH(KeepV.Len());
  for (TInt64 i = 0; i < KeepV.Len(); i++) {
    KeepH.AddKey(KeepV[i]);
  }

  TRowIteratorWithRemove RowI = BegRIWR();
  TInt64 KeepSize = 0;
  while (RowI.GetNextRowIdx() != Last) {
    if (KeepSize < KeepV.Len()) {
      if (KeepH.IsKey(RowI.GetNextRowIdx())) {
        KeepSize++;
        RowI++;
      } else {
        RowI.RemoveNext();
      }
    } else {
      // Covered all of KeepV. Remove the rest of the rows.
      // Current RowI.CurrRowIdx is the last element of KeepV.
      RowI.RemoveNext();
    }
  }
  LastValidRow = KeepV[KeepV.Len()-1];
}

void TTable::GetPartitionRanges(TIntPr64V& Partitions, TInt64 NumPartitions) const {
  TInt64 PartitionSize = NumValidRows / (NumPartitions);
  if (NumValidRows % NumPartitions != 0) PartitionSize++;
  if (PartitionSize < 10) {
    PartitionSize = 10;
    NumPartitions = NumValidRows / PartitionSize;
  }
  Partitions.Reserve(NumPartitions+1);

  TInt64 currRow = FirstValidRow;
  TInt64 currStart = currRow;
  if (IsNextDirty) {
    TInt64 currCount = PartitionSize;
    while (currRow != TTable::Last) {
      if (currCount == 0) {
        Partitions.Add(TInt64Pr(currStart, currRow));
        currStart = currRow;
        currCount = PartitionSize;
      }
      currRow = Next[currRow];
      currCount--;
    }
    Partitions.Add(TInt64Pr(currStart, currRow));
  } else {
    // Optimize for the case when rows are logically in sequence.
    currRow += PartitionSize;
    while (currRow != TTable::Last && currRow < Next.Len()) {
      if (Next[currRow] == TTable::Invalid) { currRow++; continue; }
      Partitions.Add(TInt64Pr(currStart, currRow));
      currStart = currRow;
      currRow += PartitionSize;
    }
    Partitions.Add(TInt64Pr(currStart, TTable::Last));
  }
  //printf("Num partitions: %d\n", Partitions.Len());
}

/*****  Grouping Utility functions ****/
void TTable::GroupingSanityCheck(const TStr& GroupBy, const TAttrType& AttrType) const {
  if (!IsColName(GroupBy)) {
    TExcept::Throw("no such column " + GroupBy);
  }
  if (GetColType(GroupBy) != AttrType) {
    TExcept::Throw(GroupBy + " values are not of expected type");
  }
}

#ifdef GCC_ATOMIC
void TTable::GroupByIntColMP(const TStr& GroupBy, THashMP<TInt64, TInt64V, int64>& Grouping, TBool UsePhysicalIds) const {
  timeval timer0;
  gettimeofday(&timer0, NULL);
  double t1 = timer0.tv_sec + (timer0.tv_usec/1000000.0);
  //printf("X\n");
  TInt64 IdColIdx = GetColIdx(IdColName);
  TInt64 GroupByColIdx = GetColIdx(GroupBy);
  if(!UsePhysicalIds && IdColIdx < 0){
    TExcept::Throw("Grouping: Either use physical row ids, or have an id column");
  }
  //double startFn = omp_get_wtime();
  GroupingSanityCheck(GroupBy, atInt);
  TIntPr64V Partitions;
  GetPartitionRanges(Partitions, 8*CHUNKS_PER_THREAD);
  //TInt64 PartitionSize = Partitions[0].GetVal2()-Partitions[0].GetVal1()+1;
  //double endPart = omp_get_wtime();
  //printf("Partition time = %f\n", endPart-startFn);

  Grouping.Gen(NumValidRows);
  //double endGen = omp_get_wtime();
  //printf("Gen time = %f\n", endGen-endPart);
  //printf("S\n");
  // TODO crashes when enabled!
  //#pragma omp parallel for schedule(dynamic, CHUNKS_PER_THREAD) //num_threads(1)
  for (int64 i = 0; i < Partitions.Len(); i++){
    TRowIterator RowI(Partitions[i].GetVal1(), this);
    TRowIterator EndI(Partitions[i].GetVal2(), this);
    while (RowI < EndI) {
      TInt64 idx = UsePhysicalIds ? RowI.GetRowIdx() : RowI.GetIntAttr(IdColIdx);
      //printf("updating grouping with key = %d, row_id = %d\n", RowI.GetIntAttr(GroupBy).Val, idx.Val);
      UpdateGrouping<TInt64>(Grouping, RowI.GetIntAttr(GroupByColIdx), idx);
      RowI++;
    }
  }
  gettimeofday(&timer0, NULL);
  double t2 = timer0.tv_sec + (timer0.tv_usec/1000000.0);
  printf("Grouping time: %f\n", t2 - t1);
  //double endAdd = omp_get_wtime();
  //printf("Add time = %f\n", endAdd-endGen);
}
#endif // GCC_ATOMIC

void TTable::Unique(const TStr& Col) {
  TInt64V RemainingRows;
  TStr NCol = NormalizeColName(Col);
  switch (GetColType(NCol)) {
    case atInt: {
      TIntInt64V64H Grouping;
      GroupByIntCol(NCol, Grouping, TInt64V(), true, true);
      for (TIntInt64V64H::TIter it = Grouping.BegI(); it < Grouping.EndI(); it++) {
        RemainingRows.Add(it->Dat[0]);
      }
      break;
    }
    case atFlt: {
      THash<TFlt,TInt64V, int64> Grouping;
      GroupByFltCol(NCol, Grouping, TInt64V(), true, true);
      for (THash<TFlt,TInt64V, int64>::TIter it = Grouping.BegI(); it < Grouping.EndI(); it++) {
        RemainingRows.Add(it->Dat[0]);
      }
      break;
    }
    case atStr: {
      TIntInt64V64H Grouping;
      GroupByStrCol(NCol, Grouping, TInt64V(), true, true);
      for (TIntInt64V64H::TIter it = Grouping.BegI(); it < Grouping.EndI(); it++) {
        RemainingRows.Add(it->Dat[0]);
      }
      break;
    }
  }
  KeepSortedRows(RemainingRows);
}

void TTable::Unique(const TStr64V& Cols, TBool Ordered) {
  if (Cols.Len() == 1) {
    Unique(Cols[0]);
    return;
  }
  TStr64V NCols = NormalizeColNameV(Cols);
  //for (TStr NColNm : NCols) {
  //    AssertR(ColTypeMap.IsKey(NColNm), "Unknown column " + NColNm);
  //}
  THash<TGroupKey, TPair<TInt64, TInt64V>, int64 > Grouping;
  TInt64V UniqueVec;
  GroupAux(NCols, Grouping, Ordered, "", true, UniqueVec, true);
  KeepSortedRows(UniqueVec);
}

void TTable::StoreGroupCol(const TStr& GroupColName, const TVec<TPair<TInt64, TInt64>, int64 >& GroupAndRowIds) {
  // Add a column where the value of the i'th row is the group id of row i.
  IntCols.Add(TInt64V(NumRows));
  TInt64 L = IntCols.Len();
  AddColType(GroupColName, atInt, L-1);
  // Store group id for each row.
  for (TInt64 i = 0; i < GroupAndRowIds.Len(); i++) {
    IntCols[L-1][GroupAndRowIds[i].Val2] = GroupAndRowIds[i].Val1;
  }
}

// Core grouping logic.
void TTable::GroupAux(const TStr64V& GroupBy, THash<TGroupKey, TPair<TInt64, TInt64V>, int64 >& Grouping,
 TBool Ordered, const TStr& GroupColName, TBool KeepUnique, TInt64V& UniqueVec, TBool UsePhysicalIds) {
  TInt64 IdColIdx = GetColIdx(IdColName);
  if(!UsePhysicalIds && IdColIdx < 0){
    TExcept::Throw("Grouping: Either use physical row ids, or have an id column");
  }
  TInt64V IntGroupByCols;
  TInt64V FltGroupByCols;
  TInt64V StrGroupByCols;
  // get indices for each column type
  for (TInt64 c = 0; c < GroupBy.Len(); c++) {
    //printf("GroupBy col %d: %s\n", c.Val, GroupBy[c].CStr());
    if (!IsColName(GroupBy[c])) {
      TExcept::Throw("no such column " + GroupBy[c]);
    }

    TPair<TAttrType, TInt64> ColType = GetColTypeMap(GroupBy[c]);
    switch (ColType.Val1) {
      case atInt:
        IntGroupByCols.Add(ColType.Val2);
        break;
      case atFlt:
        FltGroupByCols.Add(ColType.Val2);
        break;
      case atStr:
        StrGroupByCols.Add(ColType.Val2);
        break;
    }
  }

  TInt64 IKLen = IntGroupByCols.Len();
  TInt64 FKLen = FltGroupByCols.Len();
  TInt64 SKLen = StrGroupByCols.Len();

  TInt64 GroupNum = 0;
  TVec<TPair<TInt64, TInt64>, int64> GroupAndRowIds;
  //printf("done GroupAux initialization\n");

  // iterate over rows
  for (TRowIterator it = BegRI(); it < EndRI(); it++) {
    TInt64V IKey(IKLen + SKLen, 0);
    TFlt64V FKey(FKLen, 0);
    TInt64V SKey(SKLen, 0);

    // find group key
    for (TInt64 c = 0; c < IKLen; c++) {
      IKey.Add(it.GetIntAttr(IntGroupByCols[c]));
    }
    for (TInt64 c = 0; c < FKLen; c++) {
      FKey.Add(it.GetFltAttr(FltGroupByCols[c]));
    }
    for (TInt64 c = 0; c < SKLen; c++) {
      SKey.Add(it.GetStrMapById(StrGroupByCols[c]));
    }
    if (!Ordered) {
      if (IKLen > 0) { IKey.ISort(0, IKey.Len()-1, true); }
      if (FKLen > 0) { FKey.ISort(0, FKey.Len()-1, true); }
      if (SKLen > 0) { SKey.ISort(0, SKey.Len()-1, true); }
    }
    for (TInt64 c = 0; c < SKLen; c++) {
      IKey.Add(SKey[c]);
    }

    // look for group matching the key
    TGroupKey GroupKey = TGroupKey(IKey, FKey);

    TInt64 RowIdx = it.GetRowIdx();
    TInt64 idx = UsePhysicalIds ? it.GetRowIdx() : IntCols[IdColIdx][it.GetRowIdx()];
    if (!Grouping.IsKey(GroupKey)) {
      // Grouping key hasn't been seen before, create a new group
      TPair<TInt64, TInt64V> NewGroup;
      NewGroup.Val1 = GroupNum;
      NewGroup.Val2.Add(idx);
      Grouping.AddDat(GroupKey, NewGroup);
      if (GroupColName != "") {
        GroupAndRowIds.Add(TPair<TInt64, TInt64>(GroupNum, RowIdx));
      }
      if (KeepUnique) {
        UniqueVec.Add(idx);
      }
      GroupNum++;
    } else {
      // Grouping key has been seen before, update corresponding group
      if (!KeepUnique) {
        TPair<TInt64, TInt64V>& NewGroup = Grouping.GetDat(GroupKey);
        NewGroup.Val2.Add(idx);
        if (GroupColName != "") {
          GroupAndRowIds.Add(TPair<TInt64, TInt64>(NewGroup.Val1, RowIdx));
        }
      }
    }
  }
  // printf("KeepUnique: %d\n", KeepUnique.Val);
  // update group mapping
  if (!KeepUnique) {
    GroupStmt Stmt(NormalizeColNameV(GroupBy), Ordered, UsePhysicalIds);
    GroupStmtNames.AddDat(GroupColName, Stmt);
    GroupIDMapping.AddKey(Stmt);
    GroupMapping.AddKey(Stmt);
    //printf("Adding statement: ");
    //Stmt.Print();
    for (THash<TGroupKey, TPair<TInt64, TInt64V>, int64 >::TIter it = Grouping.BegI(); it < Grouping.EndI(); it++) {
      TGroupKey key = it.GetKey();
      TPair<TInt64, TInt64V> group = it.GetDat();
      GroupIDMapping.GetDat(Stmt).AddDat(group.Val1, TGroupKey(key));
      GroupMapping.GetDat(Stmt).AddDat(TGroupKey(key), TInt64V(group.Val2));
    }
  }

  // add a column to the table
  if (GroupColName != "") {
    StoreGroupCol(GroupColName, GroupAndRowIds);
    AddSchemaCol(GroupColName, atInt);  // update schema
  }
}

/*
// Core grouping logic.
#ifdef USE_OPENMP
void TTable::GroupAuxMP(const TStrV& GroupBy, THashGenericMP<TGroupKey, TPair<TInt, TIntV> >& Grouping,
 TBool Ordered, const TStr& GroupColName, TBool KeepUnique, TIntV& UniqueVec, TBool UsePhysicalIds) {
  //double startFn = omp_get_wtime();
  TIntV IntGroupByCols;
  TIntV FltGroupByCols;
  TIntV StrGroupByCols;
  // get indices for each column type
  for (TInt c = 0; c < GroupBy.Len(); c++) {
    if (!IsColName(GroupBy[c])) {
      TExcept::Throw("no such column " + GroupBy[c]);
    }

    TPair<TAttrType, TInt> ColType = GetColTypeMap(GroupBy[c]);
    switch (ColType.Val1) {
      case atInt:
        IntGroupByCols.Add(ColType.Val2);
        break;
      case atFlt:
        FltGroupByCols.Add(ColType.Val2);
        break;
      case atStr:
        StrGroupByCols.Add(ColType.Val2);
        break;
    }
  }

  TInt IKLen = IntGroupByCols.Len();
  TInt FKLen = FltGroupByCols.Len();
  TInt SKLen = StrGroupByCols.Len();

  TInt GroupNum = 0;
  TInt IdColIdx = GetColIdx(IdColName);

  //double endInit = omp_get_wtime();
  //printf("Init time = %f\n", endInit-startFn);

  TVec<TPair<TInt, TInt> > GroupAndRowIds;

  // iterate over rows
  for (TRowIterator it = BegRI(); it < EndRI(); it++) {
    TIntV IKey(IKLen + SKLen, 0);
    TFltV FKey(FKLen, 0);
    TIntV SKey(SKLen, 0);

    // find group key
    for (TInt c = 0; c < IKLen; c++) {
      IKey.Add(it.GetIntAttr(IntGroupByCols[c]));
    }
    for (TInt c = 0; c < FKLen; c++) {
      FKey.Add(it.GetFltAttr(FltGroupByCols[c]));
    }
    for (TInt c = 0; c < SKLen; c++) {
      SKey.Add(it.GetStrMapById(StrGroupByCols[c]));
    }
    if (!Ordered) {
      if (IKLen > 0) { IKey.ISort(0, IKey.Len()-1, true); }
      if (FKLen > 0) { FKey.ISort(0, FKey.Len()-1, true); }
      if (SKLen > 0) { SKey.ISort(0, SKey.Len()-1, true); }
    }
    for (TInt c = 0; c < SKLen; c++) {
      IKey.Add(SKey[c]);
    }

    // look for group matching the key
    TGroupKey GroupKey = TGroupKey(IKey, FKey);

    TInt RowIdx = it.GetRowIdx();
    if (!Grouping.IsKey(GroupKey)) {
      // Grouping key hasn't been seen before, create a new group
      TPair<TInt, TIntV> NewGroup;
      NewGroup.Val1 = GroupNum;
      if(IdColIdx > 0){
        NewGroup.Val2.Add(IntCols[IdColIdx][RowIdx]);
      }
      Grouping.AddDat(GroupKey, NewGroup);
      if (GroupColName != "") {
        GroupAndRowIds.Add(TPair<TInt, TInt>(GroupNum, RowIdx));
      }
      if (KeepUnique) {
        UniqueVec.Add(RowIdx);
      }
      GroupNum++;
    } else {
      // Grouping key has been seen before, update corresponding group
      if (!KeepUnique) {
        TPair<TInt, TIntV>& NewGroup = Grouping.GetDat(GroupKey);
        if(IdColIdx > 0){
          NewGroup.Val2.Add(IntCols[IdColIdx][RowIdx]);
        }
        if (GroupColName != "") {
          GroupAndRowIds.Add(TPair<TInt, TInt>(NewGroup.Val1, RowIdx));
        }
      }
    }
  }

  //double endIter = omp_get_wtime();
  //printf("Iter time = %f\n", endIter-endInit);

  // update group mapping
  if (!KeepUnique) {
    TPair<TStrV, TBool> GroupStmt(GroupBy, Ordered);
    GroupStmtNames.AddDat(GroupColName, GroupStmt);
    GroupIDMapping.AddDat(GroupStmt);
    GroupMapping.AddDat(GroupStmt);
    for (THash<TGroupKey, TPair<TInt, TIntV> >::TIter it = Grouping.BegI(); it < Grouping.EndI(); it++) {
      TGroupKey key = it.GetKey();
      TPair<TInt, TIntV> group = it.GetDat();
      GroupIDMapping.GetDat(GroupStmt).AddDat(group.Val1, key);
      GroupMapping.GetDat(GroupStmt).AddDat(key, group.Val2);
    }
  }

  //double endMapping = omp_get_wtime();
  //printf("Mapping time = %f\n", endMapping-endIter);

  // add a column to the table
  if (GroupColName != "") {
    StoreGroupCol(GroupColName, GroupAndRowIds);
    AddSchemaCol(GroupColName, atInt);  // update schema
  }

  //double endStore = omp_get_wtime();
  //printf("Store time = %f\n", endStore-endMapping);
}
#endif // USE_OPENMP
*/

void TTable::Group(const TStr64V& GroupBy, const TStr& GroupColName, TBool Ordered, TBool UsePhysicalIds) {
  TStr64V NGroupBy = NormalizeColNameV(GroupBy);
  TStr NGroupColName = NormalizeColName(GroupColName);
  TInt64V UniqueVec;
  THash<TGroupKey, TPair<TInt64, TInt64V>, int64> Grouping;
  GroupAux(NGroupBy, Grouping, Ordered, NGroupColName, false, UniqueVec, UsePhysicalIds);
}

void TTable::InvalidatePhysicalGroupings(){
  //TODO
}

void TTable::InvalidateAffectedGroupings(const TStr& Attr){
  //TODO
}

void TTable::Aggregate(const TStr64V& GroupByAttrs, TAttrAggr AggOp,
 const TStr& ValAttr, const TStr& ResAttr, TBool Ordered) {

  for (TInt64 c = 0; c < GroupByAttrs.Len(); c++) {
    if (!IsColName(GroupByAttrs[c])) {
      TExcept::Throw("no such column " + GroupByAttrs[c]);
    }
  }

  if (GetNumValidRows() == 0) { return; }

  // double startFn = omp_get_wtime();
  TStr64V NGroupByAttrs = NormalizeColNameV(GroupByAttrs);
  TBool UsePhysicalIds = (GetColIdx(IdColName) < 0);

  THash<TInt64,TInt64V, int64> GroupByIntMapping;
  THash<TFlt,TInt64V, int64> GroupByFltMapping;
  THash<TInt64,TInt64V, int64> GroupByStrMapping;
  THash<TGroupKey,TInt64V, int64> Mapping;
#ifdef GCC_ATOMIC
  THashMP<TInt64,TInt64V, int64> GroupByIntMapping_MP(NumValidRows);
  TInt64V GroupByIntMPKeys(NumValidRows);
#endif
  TInt64 NumOfGroups = 0;
  TInt64 GroupingCase = 0;

  // check if grouping already exists
  GroupStmt Stmt(NGroupByAttrs, Ordered, UsePhysicalIds);
  if (GroupMapping.IsKey(Stmt)) {
    Mapping = GroupMapping.GetDat(Stmt);
    NumOfGroups = Mapping.Len();
  } else{
    if(NGroupByAttrs.Len() == 1){
      switch(GetColType(NGroupByAttrs[0])){
        case atInt:
#ifdef GCC_ATOMIC
          if(GetMP()){
            GroupByIntColMP(NGroupByAttrs[0], GroupByIntMapping_MP, UsePhysicalIds);
            int64 x = 0;
            for(THashMP<TInt64,TInt64V, int64>::TIter it = GroupByIntMapping_MP.BegI(); it < GroupByIntMapping_MP.EndI(); it++){
                GroupByIntMPKeys[x] = it.GetKey();
                x++;

                //printf("%d --> ", it.GetKey().Val);
                //TIntV& V = it.GetDat();
                //for(int i = 0; i < V.Len(); i++){
                //  printf(" %d", V[i].Val);
                //}
                //printf("\n");

            }
            NumOfGroups = x;
            GroupingCase = 4;
            //printf("Number of groups: %d\n", NumOfGroups.Val);
            break;
          }
#endif // GCC_ATOMIC
          GroupByIntCol(NGroupByAttrs[0], GroupByIntMapping, TInt64V(), true, UsePhysicalIds);
          NumOfGroups = GroupByIntMapping.Len();
          GroupingCase = 1;
          break;
        case atFlt:
          GroupByFltCol(NGroupByAttrs[0], GroupByFltMapping, TInt64V(), true, UsePhysicalIds);
          NumOfGroups = GroupByFltMapping.Len();
          GroupingCase = 2;
          break;
        case atStr:
          GroupByStrCol(NGroupByAttrs[0], GroupByStrMapping, TInt64V(), true, UsePhysicalIds);
          NumOfGroups = GroupByStrMapping.Len();
          GroupingCase = 3;
          break;
      }
    }
    else{
      TInt64V UniqueVector;
      THash<TGroupKey, TPair<TInt64, TInt64V>, int64 > Mapping_aux;
      GroupAux(NGroupByAttrs, Mapping_aux, Ordered, "", false, UniqueVector, UsePhysicalIds);
      for(THash<TGroupKey, TPair<TInt64, TInt64V>, int64 >::TIter it = Mapping_aux.BegI(); it < Mapping_aux.EndI(); it++){
        Mapping.AddDat(it.GetKey(), it.GetDat().Val2);
      }
      NumOfGroups = Mapping.Len();
    }
  }

  // double endGroup = omp_get_wtime();
  // printf("Group time = %f\n", endGroup-startFn);
  TAttrType T = (AggOp == aaCount) ? atInt : GetColType(ValAttr);

  // add column corresponding to result attribute type
  if (T == atInt) { AddIntCol(ResAttr); }
  else if (T == atFlt) { AddFltCol(ResAttr); }
  else {
      // Count is the only aggregation operation handled for Str
      TExcept::Throw("Invalid aggregation for Str type!");
  }
  TInt64 ColIdx = GetColIdx(ResAttr);
  TInt64 AggrColIdx = GetColIdx(ValAttr);

  // double endAdd = omp_get_wtime();
  // printf("AddCol time = %f\n", endAdd-endGroup);

#ifdef USE_OPENMP
  #pragma omp parallel for schedule(dynamic)
#endif
  for (int64 g = 0; g < NumOfGroups; g++) {
    TInt64V* GroupRows = NULL;
    switch(GroupingCase){
      case 0:
        GroupRows = & Mapping.GetDat(Mapping.GetKey(g));
        break;
      case 1:
        GroupRows = & GroupByIntMapping.GetDat(GroupByIntMapping.GetKey(g));
        break;
      case 2:
        GroupRows = & GroupByIntMapping.GetDat(GroupByIntMapping.GetKey(g));
        break;
        case 3:
        GroupRows = & GroupByStrMapping.GetDat(GroupByStrMapping.GetKey(g));
        break;
      case 4:
#ifdef GCC_ATOMIC
        GroupRows = & GroupByIntMapping_MP.GetDat(GroupByIntMPKeys[g]);
#endif
        break;
    }

    // find valid rows of group
    /*
    TIntV ValidRows;
    for (TInt i = 0; i < GroupRows.Len(); i++) {
      // TODO: This should not be necessary
      if (!RowIdMap.IsKey(GroupRows[i])) { continue; }
      TInt RowId = RowIdMap.GetDat(GroupRows[i]);
      // GroupRows has physical row indices
      if (RowId != Invalid) { ValidRows.Add(RowId); }
    }
    */
    TInt64V& ValidRows = *GroupRows;
    TInt64 sz = ValidRows.Len();
    if (sz <= 0) continue;
    // Count is handled separately (other operations have aggregation policies defined in a template)
    if (AggOp == aaCount) {
      for (TInt64 i = 0; i < sz; i++) { IntCols[ColIdx][ValidRows[i]] = sz; }
    } else {
      // aggregate based on column type
      if (T == atInt) {
        TInt64V V;
        for (TInt64 i = 0; i < sz; i++) { V.Add(IntCols[AggrColIdx][ValidRows[i]]); }
        TInt64 Res = AggregateVector<TInt64>(V, AggOp);
        if (AggOp == aaMean) { Res = Res / sz; }
        for (TInt64 i = 0; i < sz; i++) { IntCols[ColIdx][ValidRows[i]] = Res; }
      } else if (T == atFlt) {
        TFlt64V V;
        for (TInt64 i = 0; i < sz; i++) { V.Add(FltCols[AggrColIdx][ValidRows[i]]); }
        TFlt Res = AggregateVector<TFlt>(V, AggOp);
        if (AggOp == aaMean) { Res /= sz; }
        for (TInt64 i = 0; i < sz; i++) { FltCols[ColIdx][ValidRows[i]] = Res; }
      }
    }
  }
  // double endIter = omp_get_wtime();
  // printf("Iter time = %f\n", endIter-endAdd);
}

void TTable::AggregateCols(const TStr64V& AggrAttrs, TAttrAggr AggOp, const TStr& ResAttr) {
  TVec<TPair<TAttrType, TInt64>, int64 >Info;
  for (TInt64 i = 0; i < AggrAttrs.Len(); i++) {
    Info.Add(GetColTypeMap(AggrAttrs[i]));
    if (Info[i].Val1 != Info[0].Val1) {
      TExcept::Throw("AggregateCols: Aggregation attributes must have the same type");
    }
  }

  if (Info[0].Val1 == atInt) {
    AddIntCol(ResAttr);
    TInt64 ResIdx = GetColIdx(ResAttr);

    for (TRowIterator RI = BegRI(); RI < EndRI(); RI++) {
      TInt64 RowIdx = RI.GetRowIdx();
      TInt64V V;
      for (TInt64 i = 0; i < AggrAttrs.Len(); i++) {
        V.Add(IntCols[Info[i].Val2][RowIdx]);
      }
      IntCols[ResIdx][RowIdx] = AggregateVector<TInt64>(V, AggOp);
    }
  } else if (Info[0].Val1 == atFlt) {
    AddFltCol(ResAttr);
    TInt64 ResIdx = GetColIdx(ResAttr);

    for (TRowIterator RI = BegRI(); RI < EndRI(); RI++) {
      TInt64 RowIdx = RI.GetRowIdx();
      TFlt64V V;
      for (TInt64 i = 0; i < AggrAttrs.Len(); i++) {
        V.Add(FltCols[Info[i].Val2][RowIdx]);
      }
      FltCols[ResIdx][RowIdx] = AggregateVector<TFlt>(V, AggOp);
    }
  } else {
    TExcept::Throw("AggregateCols: Only Int and Flt aggregation supported right now");
  }
}

void TTable::PrintGrouping(const THash<TGroupKey, TInt64V, int64>& Mapping) const{
  for(THash<TGroupKey, TInt64V, int64>::TIter it = Mapping.BegI(); it < Mapping.EndI(); it++){
      TGroupKey gk = it.GetKey();
      TInt64V ik = gk.Val1;
      TFlt64V fk = gk.Val2;
      for(int64 i = 0; i < ik.Len(); i++){ printf("%s ",TInt64::GetStr(ik[i].Val).CStr());}
      for(int64 i = 0; i < fk.Len(); i++){ printf("%f ",fk[i].Val);}
      printf("-->");
      TInt64V v = it.GetDat();
      for(int64 i = 0; i < v.Len(); i++){ printf("%s ",TInt64::GetStr(v[i].Val).CStr());}
      printf("\n");
    }
}

void TTable::Count(const TStr& CountColName, const TStr& Col) {
  TStr64V GroupByAttrs;
  GroupByAttrs.Add(CountColName);
  Aggregate(GroupByAttrs, aaCount, "", Col);
}

TVec<PTable, int64> TTable::SpliceByGroup(const TStr64V& GroupBy, TBool Ordered) {
  TStr64V NGroupBy = NormalizeColNameV(GroupBy);
  TInt64V UniqueVec;
  THash<TGroupKey, TPair<TInt64, TInt64V>, int64 >Grouping;
  TVec<PTable, int64> Result;

  Schema NewSchema;
  for (TInt64 c = 0; c < Sch.Len(); c++) {
    if (Sch[c].Val1 != GetIdColName()) {
      NewSchema.Add(Sch[c]);
    }
  }
  const bool UsePhysicalIds = GetIdColName().Empty();

  GroupAux(NGroupBy, Grouping, Ordered, "", false, UniqueVec);

  TInt64 cnt = 0;
  // iterate over groups
  for (THash<TGroupKey, TPair<TInt64, TInt64V>, int64 >::TIter it = Grouping.BegI(); it != Grouping.EndI(); it++) {
    PTable GroupTable = TTable::New(NewSchema, Context);

    TVec<TPair<TAttrType, TInt64>, int64 > ColInfo;
    TInt64V V;
    for (TInt64 i = 0; i < Sch.Len(); i++) {
      if (Sch[i].Val1 == GetIdColName()) {
        ColInfo.Add(TPair<TAttrType, TInt64>(atInt, -1));
      } else {
        ColInfo.Add(GroupTable->GetColTypeMap(Sch[i].Val1));
      }
      V.Add(GetColIdx(Sch[i].Val1));
    }

    TInt64V& Rows = it.GetDat().Val2;

    // iterate over rows in group
    for (TInt64 i = 0; i < Rows.Len(); i++) {
      // convert from permanent ID to row ID
      TInt64 RowIdx = UsePhysicalIds ? Rows[i] : RowIdMap.GetDat(Rows[i]);

      // iterate over schema
      for (TInt64 c = 0; c < Sch.Len(); c++) {
        TPair<TAttrType, TInt64> Info = ColInfo[c];
        TInt64 ColIdx = Info.Val2;

        if (ColIdx == -1) { continue; }

        // add row to new group
        switch (Info.Val1) {
          case atInt:
            GroupTable->IntCols[ColIdx].Add(IntCols[V[c]][RowIdx]);
            break;
          case atFlt:
            GroupTable->FltCols[ColIdx].Add(FltCols[V[c]][RowIdx]);
            break;
          case atStr:
            GroupTable->StrColMaps[ColIdx].Add(StrColMaps[V[c]][RowIdx]);
            break;
        }

      }
      if (GroupTable->LastValidRow >= 0) {
        GroupTable->Next[GroupTable->LastValidRow] = GroupTable->NumRows;
      }
      GroupTable->Next.Add(GroupTable->Last);
      GroupTable->LastValidRow = GroupTable->NumRows;

      GroupTable->NumRows++;
      GroupTable->NumValidRows++;
    }
    GroupTable->InitIds();
    Result.Add(GroupTable);

    cnt += 1;
  }
  return Result;
}

void TTable::InitIds() {
  IdColName = "_id";
  //Assert(NumRows == NumValidRows);
  AddIdColumn(IdColName);
}

void TTable::Reindex() {
  RowIdMap.Clr();
  TInt64 IdColIdx = GetColIdx(IdColName);
  TInt64 IdCnt = 0;
  for (TRowIterator RI = BegRI(); RI < EndRI(); RI++) {
    IntCols[IdColIdx][RI.GetRowIdx()] = IdCnt;
    RowIdMap.AddDat(RI.GetRowIdx(), IdCnt);
    IdCnt++;
  }
}

void TTable::AddIdColumn(const TStr& ColName) {
  //printf("NumRows: %d\n", NumRows.Val);
  TInt64 IdCol = IntCols.Add();
  IntCols[IdCol].Reserve(NumRows, NumRows);
  //printf("IdCol Reserved\n");
  TInt64 IdCnt = 0;
  RowIdMap.Clr();
  for (TRowIterator RI = BegRI(); RI < EndRI(); RI++) {
    IntCols[IdCol][RI.GetRowIdx()] = IdCnt;
    RowIdMap.AddDat(IdCnt, RI.GetRowIdx());
    IdCnt++;
  }
  AddSchemaCol(ColName, atInt);
  AddColType(ColName, atInt, IntCols.Len()-1);
}

 PTable TTable::InitializeJointTable(const TTable& Table) {
  PTable JointTable = New(Context);
  JointTable->IntCols = TVec<TInt64V, int64>(IntCols.Len() + Table.IntCols.Len() + 1);
  JointTable->FltCols = TVec<TFlt64V, int64>(FltCols.Len() + Table.FltCols.Len());
  JointTable->StrColMaps = TVec<TInt64V, int64>(StrColMaps.Len() + Table.StrColMaps.Len());
  for (TInt64 i = 0; i < Sch.Len(); i++) {
    TStr ColName = GetSchemaColName(i);
    TAttrType ColType = GetSchemaColType(i);
    TStr CName = JointTable->RenumberColName(ColName);
    TPair<TAttrType, TInt64> TypeMap = GetColTypeMap(ColName);
    JointTable->AddColType(CName, TypeMap);
    //JointTable->AddLabel(CName, ColName);
    JointTable->AddSchemaCol(CName, ColType);
  }
  for (TInt64 i = 0; i < Table.Sch.Len(); i++) {
    TStr ColName = Table.GetSchemaColName(i);
    TAttrType ColType = Table.GetSchemaColType(i);
    TStr CName = JointTable->RenumberColName(ColName);
    TPair<TAttrType, TInt64> NewDat = Table.GetColTypeMap(ColName);
    Assert(ColType == NewDat.Val1);
    // add offsets
    switch (NewDat.Val1) {
      case atInt:
        NewDat.Val2 += IntCols.Len();
        break;
      case atFlt:
        NewDat.Val2 += FltCols.Len();
        break;
      case atStr:
        NewDat.Val2 += StrColMaps.Len();
        break;
    }
    JointTable->AddColType(CName, NewDat);
    JointTable->AddSchemaCol(CName, ColType);
  }
  TStr IdColName = "_id";
  JointTable->AddColType(IdColName, atInt, IntCols.Len() + Table.IntCols.Len());
  JointTable->AddSchemaCol(IdColName, atInt);
  return JointTable;
}

void TTable::AddJointRow(const TTable& T1, const TTable& T2, TInt64 RowIdx1, TInt64 RowIdx2) {
  for (TInt64 i = 0; i < T1.IntCols.Len(); i++) {
    IntCols[i].Add(T1.IntCols[i][RowIdx1]);
  }
  for (TInt64 i = 0; i < T1.FltCols.Len(); i++) {
    FltCols[i].Add(T1.FltCols[i][RowIdx1]);
  }
  for (TInt64 i = 0; i < T1.StrColMaps.Len(); i++) {
    StrColMaps[i].Add(T1.StrColMaps[i][RowIdx1]);
  }
  TInt64 IntOffset = T1.IntCols.Len();
  TInt64 FltOffset = T1.FltCols.Len();
  TInt64 StrOffset = T1.StrColMaps.Len();
  for (TInt64 i = 0; i < T2.IntCols.Len(); i++) {
    IntCols[i+IntOffset].Add(T2.IntCols[i][RowIdx2]);
  }
  for (TInt64 i = 0; i < T2.FltCols.Len(); i++) {
    FltCols[i+FltOffset].Add(T2.FltCols[i][RowIdx2]);
  }
  for (TInt64 i = 0; i < T2.StrColMaps.Len(); i++) {
    StrColMaps[i+StrOffset].Add(T2.StrColMaps[i][RowIdx2]);
  }
  TInt64 IdOffset = IntOffset + T2.IntCols.Len();
  NumRows++;
  NumValidRows++;
  if (!Next.Empty()) {
    Next[Next.Len()-1] = NumValidRows-1;
    LastValidRow = NumValidRows-1;
  }
  Next.Add(Last);
  RowIdMap.AddDat(NumRows-1,NumRows-1);
  IntCols[IdOffset].Add(NumRows-1);
}

/// Returns Similarity based join of two tables based on a given distance metric
/// and a given threshold. Records (r1, r2) that are returned satisfy the
/// criterion: d(r1, r2) <= Threshold
PTable TTable::SimJoin(const TStr64V& Cols1, const TTable& Table, const TStr64V& Cols2, const TStr& DistanceColName, const TSimType& SimType, const TFlt& Threshold)
{
  Assert(Cols1.Len() == Cols2.Len());

  if(Cols1.Len()!=Cols2.Len()){
    TExcept::Throw("Column vectors must match in type and length");
  }

  for (TInt64 i = 0; i < Cols1.Len(); i++) {
    if(!IsColName(Cols1[i]) || !Table.IsColName(Cols2[i])){
      TExcept::Throw("Column not found in Table");
    }

    TAttrType Type1 = GetColType(Cols1[i]);
    TAttrType Type2 = GetColType(Cols2[i]);

    if(Type1!=Type2){
      TExcept::Throw("Column types on the two tables must match.");
    }

    // When supporting more distance metrics, check if the types are supported for given metric.
    if((Type1!=atInt && Type1!=atFlt) || (Type2!=atInt && Type2!=atFlt)){
      TExcept::Throw("Column type not supported. Only Flt and Int column types are supported.");
    }
  }

  // Initialize Join table and add the similarity column
  PTable JointTable = InitializeJointTable(Table);
  TFlt64V DistanceV;

  // O(n^2): Parallelize
  for(TRowIterator RowI = this->BegRI(); RowI < this->EndRI(); RowI++) {
    for(TRowIterator RowI2 = Table.BegRI(); RowI2 < Table.EndRI(); RowI2++) {
      float distance = 0;

      switch(SimType)
      {
        // Calculate the distance metric
        case L2Norm:
          for(TInt64 i = 0; i < Cols1.Len(); i++) {
            float attrVal1, attrVal2;
            attrVal1 = GetColType(Cols1[i])==atInt ? (float)RowI.GetIntAttr(Cols1[i]) : (float)RowI.GetFltAttr(Cols1[i]);
            attrVal2 = Table.GetColType(Cols2[i])==atInt ? (float)RowI2.GetIntAttr(Cols2[i]) : (float)RowI2.GetFltAttr(Cols2[i]);
            distance += pow(attrVal1 - attrVal2, 2);
          }

          distance = sqrt(distance);

          if(distance<=Threshold){
            JointTable->AddJointRow(*this, Table, RowI.GetRowIdx(), RowI2.GetRowIdx());
            DistanceV.Add(distance);
          }

          // Add row to the joint table if distance <= Threshold
          break;
        // Haversine distance to calculate the distance between two points on Earth from latitude/longitude
        case Haversine:
          {
            if(Cols1.Len()!=2){
              TExcept::Throw("Haversine disance expects exactly two attributes - latitude and longitude - in that order.");
            }

            // Block to prevent cross-initialization error from compiler
            TFlt Radius = 6373; // km
            float Latitude1  = GetColType(Cols1[0])==atInt ? (float)RowI.GetIntAttr(Cols1[0]) : (float)RowI.GetFltAttr(Cols1[0]);
            float Latitude2 = Table.GetColType(Cols2[0])==atInt ? (float)RowI2.GetIntAttr(Cols2[0]) : (float)RowI2.GetFltAttr(Cols2[0]);

            float Longitude1  = GetColType(Cols1[1])==atInt ? (float)RowI.GetIntAttr(Cols1[1]) : (float)RowI.GetFltAttr(Cols1[1]);
            float Longitude2  = Table.GetColType(Cols2[1])==atInt ? (float)RowI2.GetIntAttr(Cols2[1]) : (float)RowI2.GetFltAttr(Cols2[1]);

            Latitude1 *= static_cast<float>(M_PI/180.0);
            Latitude2 *= static_cast<float>(M_PI/180.0);
            Longitude1 *= static_cast<float>(M_PI/180.0);
            Longitude2 *= static_cast<float>(M_PI/180.0);

            float dlon = Longitude2 - Longitude1;
            float dlat = Latitude2 - Latitude1;
            float a = pow(sin(dlat/2), 2) + cos(Latitude1)*cos(Latitude2)*pow(sin(dlon/2), 2);
            float c = 2*atan2(sqrt(a), sqrt(1-a));
            distance = (static_cast<float>(Radius.Val))*c;

            if(distance<=Threshold){
              JointTable->AddJointRow(*this, Table, RowI.GetRowIdx(), RowI2.GetRowIdx());
              DistanceV.Add(distance);
            }
          }
          break;
        case L1Norm:
        case Jaccard:
          TExcept::Throw("This distance metric is not supported");
      }
    }
  }

  // Add the value for the similarity column
  JointTable->StoreFltCol(DistanceColName, DistanceV);
  JointTable->InitIds();
  return JointTable;
}

PTable TTable::SelfSimJoinPerGroup(const TStr& GroupAttr, const TStr& SimCol, const TStr& DistanceColName, const TSimType& SimType, const TFlt& Threshold)
{
  if(!IsColName(SimCol) || !IsColName(GroupAttr)){
    TExcept::Throw("No such column found in table");
  }

  PTable JointTable = New(Context);
  // Initialize the joint table - (GroupId1, GroupId2, Similarity)
  JointTable->IntCols = TVec<TInt64V, int64>(2);
  JointTable->FltCols = TVec<TFlt64V, int64>(1);

  for(TInt64 i=0;i<2;i++){
    TInt64 Suffix = i+1;
    TStr CName = "GroupId_" + Suffix.GetStr();
    TPair<TAttrType, TInt64> Group(atInt, (int64)i);
    JointTable->AddColType(CName, Group);
    JointTable->AddSchemaCol(CName, atInt);
  }

  TPair<TAttrType, TInt64> Group(atFlt, 0);
  JointTable->AddColType(DistanceColName, Group);
  JointTable->AddSchemaCol(DistanceColName, atFlt);

  THash<TInt64, THash<TInt64, TInt64, int64>, int64 > TIntHH;

  TAttrType attrType = GetColType(SimCol);
  TInt64 GroupColIdx = GetColIdx(GroupAttr);
  TInt64 SimColIdx = GetColIdx(SimCol);

  for (TRowIterator RowI = this->BegRI(); RowI < this->EndRI(); RowI++) {
    TInt64 GroupId = IntCols[GroupColIdx][RowI.GetRowIdx()];

    if(attrType==atInt || attrType==atStr)
    {
      if(!TIntHH.IsKey(GroupId)){
        THash<TInt64, TInt64, int64> TIntH;
        TIntHH.AddDat(GroupId, TIntH);
      }

      THash<TInt64, TInt64, int64>& TIntH = TIntHH.GetDat(GroupId);
      TInt64 SimAttrVal = (attrType==atInt ? IntCols[SimColIdx][RowI.GetRowIdx()] : StrColMaps[SimColIdx][RowI.GetRowIdx()]);
      TIntH.AddDat(SimAttrVal, 0);
    }
    else
    {
      TExcept::Throw("Attribute type not supported.");
    }
  }

  // Iterate through every pair of groups and calculate the distance
  for (THash<TInt64, THash<TInt64, TInt64, int64>, int64 >::TIter it1 = TIntHH.BegI(); it1 < TIntHH.EndI(); it1++) {
    THash<TInt64, TInt64, int64> Vals1H = it1.GetDat();
    TInt64 GroupId1 = it1.GetKey();

    for (THash<TInt64, THash<TInt64, TInt64, int64>, int64 >::TIter it2 = TIntHH.BegI(); it2 < TIntHH.EndI(); it2++) {
        int64 intersectionCount = 0;
        TInt64 GroupId2 = it2.GetKey();
        THash<TInt64, TInt64, int64> Vals2H = it2.GetDat();

        for(THash<TInt64, TInt64, int64>::TIter it = Vals1H.BegI(); it < Vals1H.EndI(); it++)
        {
          TInt64 Val = it.GetKey();
          if(Vals2H.IsKey(Val)){
            intersectionCount+=1;
          }
        }

        int64 unionCount = Vals1H.Len() + Vals2H.Len() - intersectionCount;
        float distance = 1.0f - (float)intersectionCount/unionCount;

        // Add a new row to the JointTable
        if(distance<=Threshold){
            JointTable->IntCols[0].Add(GroupId1);
            JointTable->IntCols[1].Add(GroupId2);
            JointTable->FltCols[0].Add(distance);
            JointTable->IncrementNext();
      }
    }
  }

  JointTable->InitIds();
  return JointTable;
}

/// SimJoinPerGroup performs SimJoin based on a set of attributes. Performs the grouping internally
/// and returns a projection of the columns on which groupby was performed along with the similarity.
PTable TTable::SelfSimJoinPerGroup(const TStr64V& GroupBy, const TStr& SimCol,
 const TStr& DistanceColName, const TSimType& SimType, const TFlt& Threshold) {
  TStr64V NGroupBy = NormalizeColNameV(GroupBy);
  TStr64V ProjectionV;

  // Only keep the GroupBy cols and the SimCol
  for(TInt64 i=0; i<GroupBy.Len(); i++)
  {
    ProjectionV.Add(GroupBy[i]);
  }

  ProjectionV.Add(SimCol);
  ProjectInPlace(ProjectionV);

  TStr CName = "Group";
  TInt64V UniqueVec;
  THash<TGroupKey, TPair<TInt64, TInt64V>, int64 > Grouping;
  GroupAux(NGroupBy, Grouping, false, CName, false, UniqueVec);
  PTable GroupJointTable = SelfSimJoinPerGroup(CName, SimCol, DistanceColName, SimType, Threshold);
  PTable JointTable = InitializeJointTable(*this);

  // Hash of groupid to any arbitrary row of that group. Arbitrary because the GroupBy
  // columns within that group are the same, so we can choose any one.
  THash<TInt64, TInt64, int64> GroupIdH;

  for(THash<TGroupKey, TPair<TInt64, TInt64V>, int64 >::TIter it=Grouping.BegI(); it<Grouping.EndI(); it++)
  {
    TPair<TInt64, TInt64V> group = it.GetDat();
    TInt64 GroupNum = group.Val1;
    TInt64V RowIds = group.Val2;

    if(!GroupIdH.IsKey(GroupNum))
    {
      TInt64 RandomRowId = RowIds[0];  // Arbitrarily select the 1st row.
      GroupIdH.AddDat(GroupNum, RandomRowId);
    }
  }

  for(TRowIterator RowI = GroupJointTable->BegRI(); RowI < GroupJointTable->EndRI(); RowI++)
  {
    // The GroupJoinTable has a well defined structure - columns 0 and 1 are GroupIds
    TInt64 GroupId1 = GroupJointTable->IntCols[0][RowI.GetRowIdx()];
    TInt64 GroupId2 = GroupJointTable->IntCols[1][RowI.GetRowIdx()];

    // Get the rows for groupid1 and groupid and arbitrary select one row
    TInt64 RowId1 = GroupIdH.GetDat(GroupId1);
    TInt64 RowId2 = GroupIdH.GetDat(GroupId2);
    JointTable->AddJointRow(*this, *this, RowId1, RowId2);
  }

  // Add the simiarlity column from the GroupJointTable - GroupJointTable has a
  // well defined structure - The first float column is the similarity;
  JointTable->StoreFltCol(DistanceColName, GroupJointTable->FltCols[0]);
  ProjectionV.Clr();
  ProjectionV.Add(DistanceColName);

  // Find the GroupBy columns in the JointTable by matching the Suffix of the Schema
  // columns with the original GroupBy columns - Note that Join renames columns.
  for(TInt64 i=0; i<GroupBy.Len(); i++){
    for(TInt64 j=0; j<JointTable->Sch.Len(); j++)
    {
      TStr ColName = JointTable->Sch[j].Val1;
      if(ColName.IsStrIn(GroupBy[i]))
      {
        ProjectionV.Add(ColName);
      }
    }
  }

  JointTable->ProjectInPlace(ProjectionV);
  JointTable->InitIds();
  return JointTable;
}

// Increments the next vector and set last, NumRows and NumValidRows.
void TTable::IncrementNext()
{
  // Advance the Next vector
  NumRows++;
  NumValidRows++;
  if (!Next.Empty()) {
    Next[Next.Len()-1] = NumValidRows-1;
    LastValidRow = NumValidRows-1;
  }
  Next.Add(Last);
}

// Q: Do we want to have any gurantees in terms of order of the 0t rows - i.e.
// ordered by "this" table row idx as primary key and "Table" row idx as secondary key
 // This means only keeping joint row indices (pairs of original row indices), sorting them
 // and adding all rows in the end. Sorting can be expensive, but we would be able to pre-allocate
 // memory for the joint table..
PTable TTable::Join(const TStr& Col1, const TTable& Table, const TStr& Col2) {
  // double startFn = omp_get_wtime();
  if (!IsColName(Col1)) {
    TExcept::Throw("no such column " + Col1);
    printf("no such column %s\n", Col1.CStr());
  }
  if (!Table.IsColName(Col2)) {
    TExcept::Throw("no such column " + Col2);
    printf("no such column %s\n", Col2.CStr());
  }
  if (GetColType(Col1) != Table.GetColType(Col2)) {
    TExcept::Throw("Trying to Join on columns of different type");
    printf("Trying to Join on columns of different type\n");
  }
  //printf("passed initial checks\n");
  // initialize result table
  PTable JointTable = InitializeJointTable(Table);
  //printf("initialized joint table\n");
  // hash smaller table (group by column)
  TAttrType ColType = GetColType(Col1);
  TBool ThisIsSmaller = (NumValidRows <= Table.NumValidRows);
  const TTable& TS = ThisIsSmaller ? *this : Table;
  const TTable& TB = ThisIsSmaller ?  Table : *this;
  TStr ColS = ThisIsSmaller ? Col1 : Col2;
  TStr ColB = ThisIsSmaller ? Col2 : Col1;
  TInt64 ColBId = ThisIsSmaller ? Table.GetColIdx(ColB) : GetColIdx(ColB);
  // double endInit = omp_get_wtime();
  // printf("Init time = %f\n", endInit-startFn);
  // iterate over the rows of the bigger table and check for "collisions"
  // with the group keys for the small table.
#ifdef GCC_ATOMIC
  /*
  if (GetMP()) {
    switch(ColType){
      case atInt:{
        //CHECK
        THashMP<TInt, TIntV> T(TS.GetNumValidRows());
        TS.GroupByIntColMP(ColS, T, true);
        // double endGroup = omp_get_wtime();
        // printf("Group time = %f\n", endGroup-endInit);

        TIntPr64V Partitions;
        TB.GetPartitionRanges(Partitions, omp_get_max_threads()*CHUNKS_PER_THREAD);
        TInt64 PartitionSize = Partitions[0].GetVal2()-Partitions[0].GetVal1()+1;
        TVec<TIntPr64V, int64> JointRowIDSet(Partitions.Len());
        // double endPart = omp_get_wtime();
        // printf("Partition time = %f\n", endPart-endGroup);

        #pragma omp parallel for schedule(dynamic, CHUNKS_PER_THREAD)
        for (int64 i = 0; i < Partitions.Len(); i++){
          //double start = omp_get_wtime();
          JointRowIDSet[i].Reserve(PartitionSize);
          TRowIterator RowI(Partitions[i].GetVal1(), &TB);
          TRowIterator EndI(Partitions[i].GetVal2(), &TB);
          while (RowI < EndI) {
            TInt64 K = RowI.GetIntAttr(ColBId);
            if(T.IsKey(K)){
              TInt64V& Group = T.GetDat(K);
              for(TInt64 j = 0; j < Group.Len(); j++){
                if(ThisIsSmaller){
                  JointRowIDSet[i].Add(TInt64Pr(Group[j], RowI.GetRowIdx()));
                } else{
                  JointRowIDSet[i].Add(TInt64Pr(RowI.GetRowIdx(), Group[j]));
                }
              }
            }
            RowI++;
          }
          //double end = omp_get_wtime();
          //printf("END: Thread %d: i = %d, start = %d, end = %d, num = %d, time = %f\n", omp_get_thread_num(), i,
          //    Partitions[i].GetVal1().Val, Partitions[i].GetVal2().Val, JointRowIDSet[i].Len(), end-start);
        }
        // double endJoin = omp_get_wtime();
        // printf("Iterate time = %f\n", endJoin-endPart);
        JointTable->AddNJointRowsMP(*this, Table, JointRowIDSet);
        // double endAdd = omp_get_wtime();
        // printf("Add time = %f\n", endAdd-endJoin);
        break;
      }
      case atFlt:{
        //CHECK
        THashMP<TFlt, TIntV> T(TS.GetNumValidRows());
        TS.GroupByFltCol(ColS, T, TIntV(), true);

        TIntPr64V Partitions;
        TB.GetPartitionRanges(Partitions, omp_get_max_threads()*CHUNKS_PER_THREAD);
        TInt64 PartitionSize = Partitions[0].GetVal2()-Partitions[0].GetVal1()+1;
        TVec<TIntPr64V, int64> JointRowIDSet(Partitions.Len());

        #pragma omp parallel for schedule(dynamic)
        for (int64 i = 0; i < Partitions.Len(); i++){
          JointRowIDSet[i].Reserve(PartitionSize);
          TRowIterator RowI(Partitions[i].GetVal1(), &TB);
          TRowIterator EndI(Partitions[i].GetVal2(), &TB);
          while (RowI < EndI) {
            TFlt K = RowI.GetFltAttr(ColBId);
            if(T.IsKey(K)){
              TInt64V& Group = T.GetDat(K);
              for(TInt64 j = 0; j < Group.Len(); j++){
                if(ThisIsSmaller){
                  JointRowIDSet[i].Add(TInt64Pr(Group[j], RowI.GetRowIdx()));
                } else{
                  JointRowIDSet[i].Add(TInt64Pr(RowI.GetRowIdx(), Group[j]));
                }
              }
            }
            RowI++;
          }
        }
        JointTable->AddNJointRowsMP(*this, Table, JointRowIDSet);
        break;
      }
      case atStr:{
        //CHECK
        THashMP<TInt, TIntV> T(TS.GetNumValidRows());
        TS.GroupByStrCol(ColS, T, TIntV(), true);

        TIntPr64V Partitions;
        TB.GetPartitionRanges(Partitions, omp_get_max_threads()*CHUNKS_PER_THREAD);
        TInt64 PartitionSize = Partitions[0].GetVal2()-Partitions[0].GetVal1()+1;
        TVec<TIntPr64V, int64> JointRowIDSet(Partitions.Len());

        #pragma omp parallel for schedule(dynamic)
        for (int64 i = 0; i < Partitions.Len(); i++){
          JointRowIDSet[i].Reserve(PartitionSize);
          TRowIterator RowI(Partitions[i].GetVal1(), &TB);
          TRowIterator EndI(Partitions[i].GetVal2(), &TB);
          while (RowI < EndI) {
            TInt64 K = RowI.GetStrMapById(ColBId);
            if(T.IsKey(K)){
              TInt64V& Group = T.GetDat(K);
              for(TInt64 j = 0; j < Group.Len(); j++){
                if(ThisIsSmaller){
                  JointRowIDSet[i].Add(TInt64Pr(Group[j], RowI.GetRowIdx()));
                } else{
                  JointRowIDSet[i].Add(TInt64Pr(RowI.GetRowIdx(), Group[j]));
                }
              }
            }
            RowI++;
          }
        }
        JointTable->AddNJointRowsMP(*this, Table, JointRowIDSet);
      }
      break;
    }
  } else {
*/
#endif // GCC_ATOMIC
    switch (ColType) {
      case atInt:{
        TIntInt64V64H T;
        TS.GroupByIntCol(ColS, T, TInt64V(), true);
        for (TRowIterator RowI = TB.BegRI(); RowI < TB.EndRI(); RowI++) {
          TInt64 K = RowI.GetIntAttr(ColBId);
          if (T.IsKey(K)) {
            TInt64V& Group = T.GetDat(K);
            for (TInt64 i = 0; i < Group.Len(); i++) {
              if (ThisIsSmaller) {
                JointTable->AddJointRow(*this, Table, Group[i], RowI.GetRowIdx());
              } else {
                JointTable->AddJointRow(*this, Table, RowI.GetRowIdx(), Group[i]);
              }
            }
          }
        }
        break;
      }
      case atFlt:{
        THash<TFlt, TInt64V, int64> T;
        TS.GroupByFltCol(ColS, T, TInt64V(), true);
        for (TRowIterator RowI = TB.BegRI(); RowI < TB.EndRI(); RowI++) {
          TFlt K = RowI.GetFltAttr(ColBId);
          if (T.IsKey(K)) {
            TInt64V& Group = T.GetDat(K);
            for (TInt64 i = 0; i < Group.Len(); i++) {
              if (ThisIsSmaller) {
                JointTable->AddJointRow(*this, Table, Group[i], RowI.GetRowIdx());
              } else {
                JointTable->AddJointRow(*this, Table, RowI.GetRowIdx(), Group[i]);
              }
            }
          }
        }
        break;
      }
      case atStr:{
        TIntInt64V64H T;
        TS.GroupByStrCol(ColS, T, TInt64V(), true);
        for (TRowIterator RowI = TB.BegRI(); RowI < TB.EndRI(); RowI++) {
          TInt64 K = RowI.GetStrMapById(ColBId);
          if (T.IsKey(K)) {
            TInt64V& Group = T.GetDat(K);
            for (TInt64 i = 0; i < Group.Len(); i++) {
              if (ThisIsSmaller) {
                JointTable->AddJointRow(*this, Table, Group[i], RowI.GetRowIdx());
              } else {
                JointTable->AddJointRow(*this, Table, RowI.GetRowIdx(), Group[i]);
              }
            }
          }
        }
      }
      break;
    }
#ifdef GCC_ATOMIC
  //}
#endif
  return JointTable;
}

void TTable::ThresholdJoinInputCorrectness(const TStr& KeyCol1, const TStr& JoinCol1, const TTable& Table,
  const TStr& KeyCol2, const TStr& JoinCol2){
  if (!IsColName(KeyCol1)) {
    printf("no such column %s\n", KeyCol1.CStr());
    TExcept::Throw("no such column " + KeyCol1);
  }
  if (!Table.IsColName(KeyCol2)) {
    printf("no such column %s\n", KeyCol2.CStr());
    TExcept::Throw("no such column " + KeyCol2);
  }
  if (!IsColName(JoinCol1)) {
    printf("no such column %s\n", JoinCol1.CStr());
    TExcept::Throw("no such column " + JoinCol1);
  }
  if (!Table.IsColName(JoinCol2)) {
    printf("no such column %s\n", JoinCol2.CStr());
    TExcept::Throw("no such column " + JoinCol2);
  }
  if (GetColType(JoinCol1) != Table.GetColType(JoinCol2)) {
    printf("Trying to Join on columns of different type\n");
    TExcept::Throw("Trying to Join on columns of different type");
  }
  if (GetColType(KeyCol1) != Table.GetColType(KeyCol2)) {
    printf("Key type mismatch\n");
    TExcept::Throw("Key type mismatch");
  }
}

void TTable::ThresholdJoinCountCollisions(const TTable& TB, const TTable& TS,
  const TIntInt64V64H& T, TInt64 JoinColIdxB, TInt64 KeyColIdxB, TInt64 KeyColIdxS,
  THash<TInt64Pr,TInt64Tr, int64>& Counters, TBool ThisIsSmaller, TAttrType JoinColType, TAttrType KeyType){
    // iterate over big table and count / record joint tuples
    for (TRowIterator RowI = TB.BegRI(); RowI < TB.EndRI(); RowI++) {
    // value to join on from big table
      TInt64 JVal = 0;
      if(JoinColType == atStr){
        JVal = RowI.GetStrMapById(JoinColIdxB);
      } else{
        JVal = RowI.GetIntAttr(JoinColIdxB);
      }
      //printf("JVal: %d\n", JVal.Val);
      if(T.IsKey(JVal)){
        // read key attribute of big table row
        TInt64 KeyB = 0;
        if(KeyType == atStr){
          KeyB = RowI.GetStrMapById(KeyColIdxB);
        } else{
          KeyB = RowI.GetIntAttr(KeyColIdxB);
        }
        // read row ids from small table with join attribute value of JVal
        const TInt64V& RelevantRows = T.GetDat(JVal);
        for(int64 i = 0; i < RelevantRows.Len(); i++){
          // read key attribute of relevant row from small table
          TInt64 KeyS = 0;
          if(KeyType == atStr){
            KeyS = TS.StrColMaps[KeyColIdxS][RelevantRows[i]];
          } else{
            KeyS = TS.IntCols[KeyColIdxS][RelevantRows[i]];
          }
          // create a pair of keys - serves as a key in Counters
          TInt64Pr Keys = ThisIsSmaller ? TInt64Pr(KeyS, KeyB) : TInt64Pr(KeyB, KeyS);
          if(Counters.IsKey(Keys)){
          // if the key pair has been seen before - increment its counter by 1
            TInt64Tr& V = Counters.GetDat(Keys);
            V.Val3 = V.Val3 + 1;
          } else{
            // if the key pair hasn't been seen before - add it with value of
            // row indices that create a joint record with this key pair
            if(ThisIsSmaller){
              Counters.AddDat(Keys, TInt64Tr(RelevantRows[i], RowI.GetRowIdx(),1));
            } else{
              Counters.AddDat(Keys, TInt64Tr(RowI.GetRowIdx(), RelevantRows[i],1));
            }
          }
        }  // end of for loop
      }  // end of if statement
    } // end of for loop
}

void TTable::ThresholdJoinCountPerJoinKeyCollisions(const TTable& TB, const TTable& TS,
  const TIntInt64V64H& T, TInt64 JoinColIdxB, TInt64 KeyColIdxB, TInt64 KeyColIdxS,
  THash<TInt64Tr,TInt64Tr, int64>& Counters, TBool ThisIsSmaller, TAttrType JoinColType, TAttrType KeyType){
    for (TRowIterator RowI = TB.BegRI(); RowI < TB.EndRI(); RowI++) {
      // value to join on from big table
      TInt64 JVal = 0;
      if(JoinColType == atStr){
        JVal = RowI.GetStrMapById(JoinColIdxB);
       } else{
        JVal = RowI.GetIntAttr(JoinColIdxB);
       }
      //printf("JVal: %d\n", JVal.Val);
      if(T.IsKey(JVal)){
        // read key attribute of big table row
        TInt64 KeyB = 0;
        if(KeyType == atStr){
          KeyB = RowI.GetStrMapById(KeyColIdxB);
        } else{
          KeyB = RowI.GetIntAttr(KeyColIdxB);
        }
        // read row ids from small table with join attribute value of JVal
        const TInt64V& RelevantRows = T.GetDat(JVal);
        for(int64 i = 0; i < RelevantRows.Len(); i++){
          // read key attribute of relevant row from small table
          TInt64 KeyS = 0;
          if(KeyType == atStr){
            KeyS = TS.StrColMaps[KeyColIdxS][RelevantRows[i]];
          } else{
            KeyS = TS.IntCols[KeyColIdxS][RelevantRows[i]];
          }
          // create a pair of keys - serves as a key in Counters
          TInt64Pr Keys = ThisIsSmaller ? TInt64Pr(KeyS, KeyB) : TInt64Pr(KeyB, KeyS);
          TInt64Tr K(Keys.Val1,Keys.Val2,JVal);
          if(Counters.IsKey(K)){
            // if the key pair has been seen before - increment its counter by 1
            TInt64Tr& V = Counters.GetDat(K);
            V.Val3 = V.Val3 + 1;
          } else{
            // if the key pair hasn't been seen before - add it with value of
            // row indices that create a joint record with this key pair
            if(ThisIsSmaller){
              Counters.AddDat(K, TInt64Tr(RelevantRows[i], RowI.GetRowIdx(),1));
            } else{
              Counters.AddDat(K, TInt64Tr(RowI.GetRowIdx(), RelevantRows[i],1));
            }
          }
        }  // end of for loop
      }  // end of if statement
    } // end of for loop
  }

PTable TTable::ThresholdJoinOutputTable(const THash<TInt64Pr,TInt64Tr, int64>& Counters, TInt64 Threshold, const TTable& Table){
  // initialize result table
  PTable JointTable = InitializeJointTable(Table);
  for(THash<TInt64Pr,TInt64Tr, int64>::TIter iter = Counters.BegI(); iter < Counters.EndI(); iter++){
    TInt64Tr& Counter = iter.GetDat();
    //printf("keys: %d, %d\n", iter.GetKey().Val1.Val, iter.GetKey().Val2.Val);
    //printf("selected rows: %d,%d, counter: %d\n", Counter.Val1.Val, Counter.Val2.Val, Counter.Val3.Val);
    if(Counter.Val3 >= Threshold){
      JointTable->AddJointRow(*this, Table, Counter.Val1, Counter.Val2);
    }
  }
  return JointTable;
}

PTable TTable::ThresholdJoinPerJoinKeyOutputTable(const THash<TInt64Tr,TInt64Tr, int64>& Counters, TInt64 Threshold, const TTable& Table){
  PTable JointTable = InitializeJointTable(Table);
  for(THash<TInt64Tr,TInt64Tr, int64>::TIter iter = Counters.BegI(); iter < Counters.EndI(); iter++){
    const TInt64Tr& Counter = iter.GetDat();
    const TInt64Tr& Keys = iter.GetKey();
    THashSet<TInt64Pr, int64> Pairs;
    if(Counter.Val3 >= Threshold){
      TInt64Pr K(Keys.Val1,Keys.Val2);
      if(!Pairs.IsKey(K)){
        Pairs.AddKey(K);
        JointTable->AddJointRow(*this, Table, Counter.Val1, Counter.Val2);
      }
    }
  }
  return JointTable;
}


// expected output: one joint tuple (R1,R2) with:
// (1) R1[KeyCol1] = K1 and R2[KeyCol2] = K2
// for every pair of keys (K1,K2) such that the number of joint tuples
// (joint on R1[JoinCol1] = R2[JointCol2]) that hold property (1) is at least Threshold
PTable TTable::ThresholdJoin(const TStr& KeyCol1, const TStr& JoinCol1, const TTable& Table,
  const TStr& KeyCol2, const TStr& JoinCol2, TInt64 Threshold, TBool PerJoinKey){
  // test input correctness
  ThresholdJoinInputCorrectness(KeyCol1, JoinCol1, Table, KeyCol2, JoinCol2);
  //printf("verified input correctness\n");
  // type of column on which we join (currently support only int)
  TAttrType JoinColType = GetColType(JoinCol1);
  // type of key column (currently support only int)
  TAttrType KeyType = GetColType(KeyCol1);
  // Determine which table is smaller
  TBool ThisIsSmaller = (NumValidRows <= Table.NumValidRows);
  const TTable& TS = ThisIsSmaller ? *this : Table;
  const TTable& TB = ThisIsSmaller ?  Table : *this;
  TStr JoinColS = JoinCol1;
  TInt64 JoinColIdxB = GetColIdx(JoinCol2);
  TInt64 KeyColIdxS = GetColIdx(KeyCol1);
  TInt64 KeyColIdxB = GetColIdx(KeyCol2);
  if(!ThisIsSmaller){
    JoinColS = JoinCol2;
    JoinColIdxB = GetColIdx(JoinCol1);
    KeyColIdxS = GetColIdx(KeyCol2);
    KeyColIdxB = GetColIdx(KeyCol1);
  }

  // debug print
  //printf("JoinColS = %d, JoinColIdxB = %d, KeyColIdxS = %d, KeyColIdxB = %d\n",
    //GetColIdx(JoinColS).Val, JoinColIdxB.Val, KeyColIdxS.Val, KeyColIdxB.Val);
  //printf("starting switch-case\n");

  if(KeyType != atInt && KeyType != atStr){
    printf("ThresholdJoin only supports integer or string key attributes\n");
    TExcept::Throw("ThresholdJoin only supports integer or string key attributes");
  }
  if(JoinColType != atInt && JoinColType != atStr){
    printf("ThresholdJoin only supports integer or string join attributes\n");
    TExcept::Throw("ThresholdJoin only supports integer or string join attributes");
  }
  //printf("starting the real stuff!\n");
  // hash the smaller table T: join col value --> physical row ids of rows with that value
  TIntInt64V64H T;
  if(JoinColType == atInt){
    TS.GroupByIntCol(JoinColS, T, TInt64V(), true);
  } else if(JoinColType == atStr){
    TS.GroupByStrCol(JoinColS, T, TInt64V(), true);
  } else{
    TExcept::Throw("ThresholdJoin only supports integer or string join attributes");
  }

 /*
  for(THash<TInt,TIntV>::TIter it = T.BegI(); it < T.EndI(); it++){
    if(JoinColType == atStr){
      printf("%s -->", Context.StringVals.GetKey(it.GetKey().Val));
    } else{
      printf("%d -->", it.GetKey().Val);
    }
    const TIntV& V = it.GetDat();
    for(int sr = 0; sr < V.Len(); sr++){
      printf(" %d", V[sr].Val);
    }
    printf("\n");
  }
  */

  // Counters: (K1,K2) --> (RowIdx1,RowIdx2, count) where K1 is a key from KeyCol1,
  // K2 is a key from Table's KeyCol2; RowIdx1 and RowIdx2 are physical row ids
  // that participates in a joint tuple that satisfies (1).
  // count is the count of joint records that satisfy (1).
  // In case of string attributes - the integer mappings of the key attribute values are used.
  if(PerJoinKey){
    //printf("PerJoinKey\n");
    THash<TInt64Tr,TInt64Tr, int64> Counters;
    ThresholdJoinCountPerJoinKeyCollisions(TB, TS, T, JoinColIdxB, KeyColIdxB, KeyColIdxS, Counters, ThisIsSmaller, JoinColType, KeyType);
    /*
    for(THash<TIntTr,TIntTr>::TIter it = Counters.BegI(); it < Counters.EndI(); it++){
      const TIntTr& K = it.GetKey();
      const TIntTr& V = it.GetDat();
      if(KeyType == atStr){
        printf("%s %s --> %d %d %d\n", Context->StringVals.GetKey(K.Val1), Context->StringVals.GetKey(K.Val2), V.Val1.Val, V.Val2.Val, V.Val3.Val);
      } else{
        printf("%d %d --> %d %d %d\n", K.Val1.Val, K.Val2.Val, V.Val1.Val, V.Val2.Val, V.Val3.Val);
      }
    }
    */
    //printf("found collisions\n");
    return ThresholdJoinPerJoinKeyOutputTable(Counters, Threshold, Table);
  } else{
    //printf("not PerJoinKey\n");
    THash<TInt64Pr,TInt64Tr, int64> Counters;
    ThresholdJoinCountCollisions(TB, TS, T, JoinColIdxB, KeyColIdxB, KeyColIdxS, Counters, ThisIsSmaller, JoinColType, KeyType);
    /*
    for(THash<TIntPr,TIntTr>::TIter it = Counters.BegI(); it < Counters.EndI(); it++){
      const TIntPr& K = it.GetKey();
      const TIntTr& V = it.GetDat();
      if(KeyType == atStr){
        printf("%s %s --> %d %d %d\n", Context->StringVals.GetKey(K.Val1), Context->StringVals.GetKey(K.Val2), V.Val1.Val, V.Val2.Val, V.Val3.Val);
      } else{
        printf("%d %d --> %d %d %d\n", K.Val1.Val, K.Val2.Val, V.Val1.Val, V.Val2.Val, V.Val3.Val);
      }
    }
    */
    //printf("found collisions\n");
    return ThresholdJoinOutputTable(Counters, Threshold, Table);
  }
}


void TTable::Select(TPredicate& Predicate, TInt64V& SelectedRows, TBool Remove) {
  TInt64V Selected;
  TStr64V RelevantCols;
  Predicate.GetVariables(RelevantCols);
  TInt64 NumRelevantCols = RelevantCols.Len();
  TVec<TAttrType, int64> ColTypes = TVec<TAttrType, int64>(NumRelevantCols);
  TInt64V ColIndices = TInt64V(NumRelevantCols);
  for (TInt64 i = 0; i < NumRelevantCols; i++) {
    ColTypes[i] = GetColType(RelevantCols[i]);
    ColIndices[i] = GetColIdx(RelevantCols[i]);
  }

  if (Remove) {
    TRowIteratorWithRemove RowI = BegRIWR();
    while (RowI.GetNextRowIdx() != Last) {
      // prepare arguments for predicate evaluation
      for (TInt64 i = 0; i < NumRelevantCols; i++) {
        switch (ColTypes[i]) {
        case atInt:
          Predicate.SetIntVal(RelevantCols[i], RowI.GetNextIntAttr(ColIndices[i]));
          break;
        case atFlt:
          Predicate.SetFltVal(RelevantCols[i], RowI.GetNextFltAttr(ColIndices[i]));
          break;
        case atStr:
          Predicate.SetStrVal(RelevantCols[i], RowI.GetNextStrAttr(ColIndices[i]));
          break;
        }
      }
      if (!Predicate.Eval()) {
        RowI.RemoveNext();
      } else {
        RowI++;
      }
    }
  } else {
    for (TRowIterator RowI = BegRI(); RowI < EndRI(); RowI++) {
      for (TInt64 i = 0; i < NumRelevantCols; i++) {
        switch (ColTypes[i]) {
        case atInt:
          Predicate.SetIntVal(RelevantCols[i], RowI.GetIntAttr(RelevantCols[i]));
          break;
        case atFlt:
          Predicate.SetFltVal(RelevantCols[i], RowI.GetFltAttr(RelevantCols[i]));
          break;
        case atStr:
          Predicate.SetStrVal(RelevantCols[i], RowI.GetStrAttr(RelevantCols[i]));
          break;
        }
      }
      if (Predicate.Eval()) { SelectedRows.Add(RowI.GetRowIdx()); }
    }
  }
}

void TTable::Classify(TPredicate& Predicate, const TStr& LabelName, const TInt64& PositiveLabel, const TInt64& NegativeLabel) {
  TInt64V SelectedRows;
  Select(Predicate, SelectedRows, false);
  ClassifyAux(SelectedRows, LabelName, PositiveLabel, NegativeLabel);
}


// Further optimization: both comparison operation and type of columns don't change between rows..
void TTable::SelectAtomic(const TStr& Col1, const TStr& Col2, TPredComp Cmp, TInt64V& SelectedRows, TBool Remove) {
  const TAttrType Ty1 = GetColType(Col1);
  const TAttrType Ty2 = GetColType(Col2);
  const TInt64 ColIdx1 = GetColIdx(Col1);
  const TInt64 ColIdx2 = GetColIdx(Col2);
  if (Ty1 != Ty2) {
    TExcept::Throw("SelectAtomic: diff types");
  }
  if (Cmp == SUBSTR || Cmp == SUPERSTR) { Assert(Ty1 == atStr); }

  if (Remove) {
    TRowIteratorWithRemove RowI = BegRIWR();
    while (RowI.GetNextRowIdx() != Last) {

      TBool Result;
      switch (Ty1) {
        case atInt:
          Result = TPredicate::EvalAtom(RowI.GetNextIntAttr(ColIdx1), RowI.GetNextIntAttr(ColIdx2), Cmp);
          break;
        case atFlt:
          Result = TPredicate::EvalAtom(RowI.GetNextFltAttr(ColIdx1), RowI.GetNextFltAttr(ColIdx2), Cmp);
          break;
        case atStr:
          Result = TPredicate::EvalStrAtom(RowI.GetNextStrAttr(ColIdx1), RowI.GetNextStrAttr(ColIdx2), Cmp);
          break;
      }

      if (!Result) {
        RowI.RemoveNext();
      } else {
        RowI++;
      }

    }
  } else {
    for (TRowIterator RowI = BegRI(); RowI < EndRI(); RowI++) {
      TBool Result;
      switch (Ty1) {
        case atInt:
          Result = TPredicate::EvalAtom(RowI.GetIntAttr(Col1), RowI.GetIntAttr(Col2), Cmp);
          break;
        case atFlt:
          Result = TPredicate::EvalAtom(RowI.GetFltAttr(Col1), RowI.GetFltAttr(Col2), Cmp);
          break;
        case atStr:
          Result = TPredicate::EvalStrAtom(RowI.GetStrAttr(Col1), RowI.GetStrAttr(Col2), Cmp);
          break;
      }
      if (Result) { SelectedRows.Add(RowI.GetRowIdx()); }
    }
  }
}

void TTable::ClassifyAtomic(const TStr& Col1, const TStr& Col2, TPredComp Cmp,
  const TStr& LabelName, const TInt64& PositiveLabel, const TInt64& NegativeLabel) {
  TInt64V SelectedRows;
  SelectAtomic(Col1, Col2, Cmp, SelectedRows, false);
  ClassifyAux(SelectedRows, LabelName, PositiveLabel, NegativeLabel);
}

void TTable::SelectAtomicConst(const TStr& Col, const TPrimitive& Val, TPredComp Cmp,
  TInt64V& SelectedRows, PTable& SelectedTable, TBool Remove, TBool Table) {
  //double startFn = omp_get_wtime();
  TStr ValTStr(Val.GetStr());
  TAttrType Type = GetColType(Col);
  TInt64 ColIdx = GetColIdx(Col);

  if (Type != Val.GetType()) {
    TExcept::Throw("SelectAtomicConst: coltype does not match const type");
  }

  if(Remove){
#ifdef USE_OPENMP
    if (GetMP()) {
      //double endInit = omp_get_wtime();
      //printf("Init time = %f\n", endInit-startFn);
      TIntPr64V Partitions;
      GetPartitionRanges(Partitions, omp_get_max_threads()*CHUNKS_PER_THREAD);
      //TInt64 PartitionSize = Partitions[0].GetVal2()-Partitions[0].GetVal1()+1;
      int64 RemoveCount = 0;
      //double endPart = omp_get_wtime();
      //printf("Partition time = %f\n", endPart-endInit);

      TIntPr64V Bounds(Partitions.Len());

      // #pragma omp parallel for schedule(dynamic, CHUNKS_PER_THREAD) reduction(+:RemoveCount) shared(Val)
      #pragma omp parallel for schedule(dynamic, CHUNKS_PER_THREAD) reduction(+:RemoveCount)
      for (int64 i = 0; i < Partitions.Len(); i++){
        //TPrimitive ThreadLocalVal(Val);
        TRowIterator RowI(Partitions[i].GetVal1(), this);
        TRowIterator EndI(Partitions[i].GetVal2(), this);
        TInt64 FirstRowIdx = TTable::Invalid;
        TInt64 LastRowIdx = TTable::Invalid;
        TBool First = true;
        while (RowI < EndI) {
          TInt64 CurrRowIdx = RowI.GetRowIdx();
          TBool Result;
          if (Type != atStr) {
            Result = RowI.CompareAtomicConst(ColIdx, Val, Cmp);
          } else {
            Result = RowI.CompareAtomicConstTStr(ColIdx, ValTStr, Cmp);
          }
          RowI++;
          if(!Result) {
            Next[CurrRowIdx] = TTable::Invalid;
            RemoveCount++;
          } else {
            if (First) { FirstRowIdx = CurrRowIdx; First = false; }
            else { Next[LastRowIdx] = CurrRowIdx; }
            LastRowIdx = CurrRowIdx;
          }
        }
        Bounds[i] = TInt64Pr(FirstRowIdx, LastRowIdx);
        //printf("Thread %d: i = %d, start = %d, end = %d\n", omp_get_thread_num(), i,
        //  Partitions[i].GetVal1().Val, Partitions[i].GetVal2().Val);
      }
      //double endIter = omp_get_wtime();
      //printf("Iter time = %f\n", endIter-endPart);

      // repair the next vector
      TInt64 CurrBound = 0;
      while (CurrBound < Bounds.Len() && Bounds[CurrBound].Val1 == TTable::Invalid) {
        CurrBound++;
      }
      if (CurrBound == Bounds.Len()) {
        // selected table is empty
        Assert(NumValidRows == RemoveCount);
        NumValidRows = 0;
        FirstValidRow = TTable::Invalid;
        LastValidRow = TTable::Invalid;
      } else {
        NumValidRows -= RemoveCount;
        FirstValidRow = Bounds[CurrBound].Val1;
        LastValidRow = Bounds[CurrBound].Val2;
        TInt64 PrevBound = CurrBound;
        CurrBound++;
        while (CurrBound < Bounds.Len()) {
          if (Bounds[CurrBound].Val1 == TTable::Invalid) { CurrBound++; continue; }
          Next[Bounds[PrevBound].Val2] = Bounds[CurrBound].Val1;
          LastValidRow = Bounds[CurrBound].Val2;
          PrevBound = CurrBound;
          CurrBound++;
        }
        Next[Bounds[PrevBound].Val2] = TTable::Last;
      }
      IsNextDirty = 1;
      //double endRepair = omp_get_wtime();
      //printf("Repair time = %f\n", endRepair-endIter);
    } else {
#endif
      TRowIteratorWithRemove RowI = BegRIWR();
      while(RowI.GetNextRowIdx() != Last){
        if (!RowI.CompareAtomicConst(ColIdx, Val, Cmp)) {
          RowI.RemoveNext();
        } else {
          RowI++;
        }
      }
      IsNextDirty = 1;
#ifdef USE_OPENMP
    }
#endif
  } else if (Table) {
#ifdef USE_OPENMP
    if (GetMP()) {
      //double endInit = omp_get_wtime();
      //printf("Init time = %f\n", endInit-startFn);
      TIntPr64V Partitions;
      GetPartitionRanges(Partitions, omp_get_max_threads()*CHUNKS_PER_THREAD);
      TInt64 PartitionSize = Partitions[0].GetVal2()-Partitions[0].GetVal1()+1;
      //double endPart = omp_get_wtime();
      //printf("Partition time = %f\n", endPart-endInit);

      int64 TotalSelectedRows = 0;
      #pragma omp parallel for schedule(dynamic, CHUNKS_PER_THREAD) reduction(+:TotalSelectedRows)
      for (int64 i = 0; i < Partitions.Len(); i++){
        TRowIterator RowI(Partitions[i].GetVal1(), this);
        TRowIterator EndI(Partitions[i].GetVal2(), this);
        while (RowI < EndI) {
          if (Type != atStr) {
            if (RowI.CompareAtomicConst(ColIdx, Val, Cmp)) {
              TotalSelectedRows++;
            }
          } else {
            if (RowI.CompareAtomicConstTStr(ColIdx, ValTStr, Cmp)) {
              TotalSelectedRows++;
            }
          }
          RowI++;
        }
      }
      //double endCount = omp_get_wtime();
      //printf("Count time = %f\n", endCount-endPart);

      SelectedTable->ResizeTable(TotalSelectedRows);
      //double endResize = omp_get_wtime();
      //printf("Resize time = %f\n", endResize-endCount);

      if (TotalSelectedRows == 0) {
        // printf("Select: Empty output!\n");
        return;
      }

      #pragma omp parallel for schedule(dynamic, CHUNKS_PER_THREAD)
      for (int64 i = 0; i < Partitions.Len(); i++){
        TInt64V LocalSelectedRows;
        LocalSelectedRows.Reserve(PartitionSize);
        TRowIterator RowI(Partitions[i].GetVal1(), this);
        TRowIterator EndI(Partitions[i].GetVal2(), this);
        while (RowI < EndI) {
          if (Type != atStr) {
            if (RowI.CompareAtomicConst(ColIdx, Val, Cmp)) {
              LocalSelectedRows.Add(RowI.GetRowIdx());
            }
          } else {
            if (RowI.CompareAtomicConstTStr(ColIdx, ValTStr, Cmp)) {
              LocalSelectedRows.Add(RowI.GetRowIdx());
            }
          }
          RowI++;
        }
        SelectedTable->AddSelectedRows(*this, LocalSelectedRows);
        //printf("Thread %d: i = %d, start = %d, end = %d\n", omp_get_thread_num(), i,
        //  Partitions[i].GetVal1().Val, Partitions[i].GetVal2().Val);
      }
      //double endIter = omp_get_wtime();
      //printf("Iter time = %f\n", endIter-endResize);

      //SelectedTable->ResizeTable(SelectedTable->GetNumValidRows());
      //double endResize2 = omp_get_wtime();
      //printf("Resize2 time = %f\n", endResize2-endIter);
      SelectedTable->SetFirstValidRow();
    } else {
#endif
      for(TRowIterator RowI = BegRI(); RowI < EndRI(); RowI++){
        if (RowI.CompareAtomicConst(ColIdx, Val, Cmp)) {
          SelectedTable->AddRow(RowI);
        }
      }
#ifdef USE_OPENMP
    }
#endif
  } else {
    for(TRowIterator RowI = BegRI(); RowI < EndRI(); RowI++){
      if (RowI.CompareAtomicConst(ColIdx, Val, Cmp)) {
        SelectedRows.Add(RowI.GetRowIdx());
      }
    }
  }
}

inline TInt64 TTable::CompareRows(TInt64 R1, TInt64 R2, const TAttrType& CompareByType, const TInt64& CompareByIndex, TBool Asc) {
  //printf("comparing rows %d %d by %s\n", R1.Val, R2.Val, CompareBy.CStr());
  switch (CompareByType) {
    case atInt:{
      if (IntCols[CompareByIndex][R1] > IntCols[CompareByIndex][R2]) { return (Asc ? 1 : -1); }
      if (IntCols[CompareByIndex][R1] < IntCols[CompareByIndex][R2]) { return (Asc ? -1 : 1); }
      return 0;
    }
    case atFlt:{
      if (FltCols[CompareByIndex][R1] > FltCols[CompareByIndex][R2]) { return (Asc ? 1 : -1); }
      if (FltCols[CompareByIndex][R1] < FltCols[CompareByIndex][R2]) { return (Asc ? -1 : 1); }
      return 0;
    }
    case atStr:{
      TStr S1 = GetStrVal(CompareByIndex, R1);
      TStr S2 = GetStrVal(CompareByIndex, R2);
      int64 CmpRes = strcmp(S1.CStr(), S2.CStr());
      return (Asc ? CmpRes : -CmpRes);
    }
  }
  // code should not come here, added to remove a compiler warning
  return 0;
}

inline TInt64 TTable::CompareRows(TInt64 R1, TInt64 R2, const TVec<TAttrType, int64>& CompareByTypes, const TInt64V& CompareByIndices, TBool Asc) {
  for (TInt64 i = 0; i < CompareByTypes.Len(); i++) {
    TInt64 res = CompareRows(R1, R2, CompareByTypes[i], CompareByIndices[i], Asc);
    if (res != 0) { return res; }
  }
  return 0;
}

void TTable::ISort(TInt64V& V, TInt64 StartIdx, TInt64 EndIdx, const TVec<TAttrType, int64>& SortByTypes, const TInt64V& SortByIndices, TBool Asc) {
  if (StartIdx < EndIdx) {
    for (TInt64 i = StartIdx+1; i <= EndIdx; i++) {
      TInt64 Val = V[i];
      TInt64 j = i;
      while ((StartIdx < j) && (CompareRows(V[j-1], Val, SortByTypes, SortByIndices, Asc) > 0)) {
        V[j] = V[j-1];
        j--;
      }
      V[j] = Val;
    }
  }
}

TInt64 TTable::GetPivot(TInt64V& V, TInt64 StartIdx, TInt64 EndIdx, const TVec<TAttrType, int64>& SortByTypes, const TInt64V& SortByIndices, TBool Asc) {
  TInt64 L = EndIdx - StartIdx + 1;
  //CHECK TInt64::GetRnd not a member
  const TInt64 Idx1 = StartIdx + TInt::GetRnd(L);
  const TInt64 Idx2 = StartIdx + TInt::GetRnd(L);
  const TInt64 Idx3 = StartIdx + TInt::GetRnd(L);
  if (CompareRows(V[Idx1], V[Idx2], SortByTypes, SortByIndices, Asc) < 0) {
    if (CompareRows(V[Idx2], V[Idx3], SortByTypes, SortByIndices, Asc) < 0) { return Idx2; }
    if (CompareRows(V[Idx1], V[Idx3], SortByTypes, SortByIndices, Asc) < 0) { return Idx3; }
    return Idx1;
  } else {
    if (CompareRows(V[Idx3], V[Idx2], SortByTypes, SortByIndices, Asc) < 0) { return Idx2; }
    if (CompareRows(V[Idx3], V[Idx1], SortByTypes, SortByIndices, Asc) < 0) { return Idx3; }
    return Idx1;
  }
}

TInt64 TTable::Partition(TInt64V& V, TInt64 StartIdx, TInt64 EndIdx, const TVec<TAttrType, int64>& SortByTypes, const TInt64V& SortByIndices, TBool Asc) {

  // test if the elements are already sorted
  TInt64 j;
  for (j = StartIdx; j < EndIdx; j++) {
    if (CompareRows(V[j], V[j+1], SortByTypes, SortByIndices, Asc) > 0) {
      break;
    }
  }
  if (j >= EndIdx) {
    return EndIdx+1;
  }

  TInt64 PivotIdx = GetPivot(V, StartIdx, EndIdx, SortByTypes, SortByIndices, Asc);
  TInt64 Pivot = V[PivotIdx];
  V.Swap(PivotIdx, EndIdx);
  TInt64 StoreIdx = StartIdx;
  for (TInt64 i = StartIdx; i < EndIdx; i++) {
    if (CompareRows(V[i], Pivot, SortByTypes, SortByIndices, Asc) <= 0) {
      V.Swap(i, StoreIdx);
      StoreIdx++;
    }
  }
  // move pivot value to its place
  V.Swap(StoreIdx, EndIdx);
  return StoreIdx;
}

void TTable::QSort(TInt64V& V, TInt64 StartIdx, TInt64 EndIdx, const TVec<TAttrType, int64>& SortByTypes, const TInt64V& SortByIndices, TBool Asc) {
  if (StartIdx < EndIdx) {
    if (EndIdx - StartIdx < 20) {
      ISort(V, StartIdx, EndIdx, SortByTypes, SortByIndices, Asc);
    } else {
      TInt64 Pivot = Partition(V, StartIdx, EndIdx, SortByTypes, SortByIndices, Asc);
      if (Pivot > EndIdx) {
        return;
      }
      // Everything <= Pivot will be in StartIdx, Pivot-1. Shrink this
      // range to ignore elements equal to the pivot in the first
      // recursive call, to optimize for the case when a lot of
      // rows are equal.
      int64 Ub = Pivot - 1;
      while (Ub >= StartIdx && CompareRows(
        V[Ub], V[Pivot], SortByTypes, SortByIndices, Asc) == 0) {
        Ub -= 1;
      }
      QSort(V, StartIdx, Ub, SortByTypes, SortByIndices, Asc);
      QSort(V, Pivot+1, EndIdx, SortByTypes, SortByIndices, Asc);
    }
  }
}

void TTable::Merge(TInt64V& V, TInt64 Idx1, TInt64 Idx2, TInt64 Idx3, const TVec<TAttrType, int64>& SortByTypes, const TInt64V& SortByIndices, TBool Asc) {
  TInt64 i = Idx1, j = Idx2;
  TInt64V SortedV;
  while  (i < Idx2 && j < Idx3) {
    if (CompareRows(V[i], V[j], SortByTypes, SortByIndices, Asc) <= 0) {
      SortedV.Add(V[i]);
      i++;
    }
    else {
      SortedV.Add(V[j]);
      j++;
    }
  }
  while (i < Idx2) {
    SortedV.Add(V[i]);
    i++;
  }
  while (j < Idx3) {
    SortedV.Add(V[j]);
    j++;
  }

  for (TInt64 sz = 0; sz < Idx3 - Idx1; sz++) {
    V[Idx1 + sz] = SortedV[sz];
  }
}

#ifdef USE_OPENMP
void TTable::QSortPar(TInt64V& V, const TVec<TAttrType, int64>& SortByTypes, const TInt64V& SortByIndices, TBool Asc) {
  TInt64 NumThreads = 8; // Setting this to 8 because that results in the fastest sorting on Madmax.
  TInt64 Sz = V.Len();
  TInt64V IndV, NextV;
  for (TInt64 i = 0; i < NumThreads; i++) {
    IndV.Add(i * (Sz / NumThreads));
  }
  IndV.Add(Sz);

  omp_set_num_threads(NumThreads);
  #pragma omp parallel for
  for (int64 i = 0; i < NumThreads; i++) {
    QSort(V, IndV[i], IndV[i+1] - 1, SortByTypes, SortByIndices, Asc);
  }

  while (NumThreads > 1) {
    omp_set_num_threads(NumThreads / 2);
    #pragma omp parallel for
    for (int64 i = 0; i < NumThreads; i += 2) {
      Merge(V, IndV[i], IndV[i+1], IndV[i+2], SortByTypes, SortByIndices, Asc);
    }

    NextV.Clr();
    for (TInt64 i = 0; i < NumThreads; i+=2) {
      NextV.Add(IndV[i]);
    }
    NextV.Add(Sz);
    IndV = NextV;

    NumThreads = NumThreads / 2;
  }
}
#endif // USE_OPENMP

void TTable::Order(const TStr64V& OrderBy, TStr OrderColName, TBool ResetRankByMSC, TBool Asc) {
  // get a vector of all valid row indices
  TInt64V ValidRows = TInt64V(NumValidRows);
  if (NumRows == NumValidRows) {
    for (TInt64 i = 0; i < NumValidRows; i++) {
      ValidRows[i] = i;
    }
  } else {
    TInt64 i = 0;
    for (TRowIterator RI = BegRI(); RI < EndRI(); RI++) {
      ValidRows[i] = RI.GetRowIdx();
      i++;
    }
  }
  TVec<TAttrType, int64> OrderByTypes(OrderBy.Len());
  TInt64V OrderByIndices(OrderBy.Len());
  for (TInt64 i = 0; i < OrderBy.Len(); i++) {
    OrderByTypes[i] = GetColType(OrderBy[i]);
    OrderByIndices[i] = GetColIdx(OrderBy[i]);
  }

  // sort that vector according to the attributes given in "OrderBy" in lexicographic order
#ifdef USE_OPENMP
  if (GetMP()) {
    QSortPar(ValidRows, OrderByTypes, OrderByIndices, Asc);
  } else {
#endif
    QSort(ValidRows, 0, NumValidRows-1, OrderByTypes, OrderByIndices, Asc);
#ifdef USE_OPENMP
  }
#endif

  // rewire Next vector
  IsNextDirty = 1;
  if (NumValidRows > 0) {
    FirstValidRow = ValidRows[0];
  } else {
    FirstValidRow = Last;
  }
  for (TInt64 i = 0; i < NumValidRows-1; i++) {
    Next[ValidRows[i]] = ValidRows[i+1];
  }
  if (NumValidRows > 0) {
    Next[ValidRows[NumValidRows-1]] = Last;
    LastValidRow = ValidRows[NumValidRows-1];
  } else {
    LastValidRow = Last;
  }

  // add rank column
  if (!OrderColName.Empty()) {
    TInt64V RankCol = TInt64V(NumRows);
    for (TInt64 i = 0; i < NumValidRows; i++) {
      RankCol[ValidRows[i]] = i;
    }
    if (ResetRankByMSC) {
      for (TInt64 i = 1; i < NumValidRows; i++) {
        TStr GroupName = OrderBy[0];
        if (GetStrVal(GroupName, ValidRows[i]) != GetStrVal(GroupName, ValidRows[i-1])) {
          RankCol[ValidRows[i]] = 0;
        } else {
          RankCol[ValidRows[i]] = RankCol[ValidRows[i-1]] + 1;
        }
      }
    }
    IntCols.Add(RankCol);
    AddSchemaCol(OrderColName, atInt);
    AddColType(OrderColName, atInt, IntCols.Len()-1);
  }
}

void TTable::Defrag() {
  TInt64 FreeIndex = 0;
  TInt64V Mapping;  // Mapping[old_index] = new_index/invalid

  TInt64 IdColIdx = GetColIdx(IdColName);

  for (TInt64 i = 0; i < Next.Len(); i++) {
    if (Next[i] != TTable::Invalid) {
      // "first row" properly set beforehand
      if (FreeIndex == 0) {
        Assert (i == FirstValidRow);
        FirstValidRow = 0;
      }

      if (Next[i] != Last) {
        Next[FreeIndex] = FreeIndex + 1;
        Mapping.Add(FreeIndex);
      } else {
        Next[FreeIndex] = Last;
        LastValidRow = FreeIndex;
        Mapping.Add(Last);
      }

      RowIdMap.AddDat(IntCols[IdColIdx][i], FreeIndex);

      for (TInt64 j = 0; j < IntCols.Len(); j++) {
        IntCols[j][FreeIndex] = IntCols[j][i];
      }
      for (TInt64 j = 0; j < FltCols.Len(); j++) {
        FltCols[j][FreeIndex] = FltCols[j][i];
      }
      for (TInt64 j = 0; j < StrColMaps.Len(); j++) {
        StrColMaps[j][FreeIndex] = StrColMaps[j][i];
      }

      FreeIndex++;
    } else {
      NumRows--;
      Mapping.Add(TTable::Invalid);
    }
  }

  // should match, or bug somewhere
  Assert (NumValidRows == NumRows);
}

void TTable::SelectFirstNRows(const TInt64& N) {
  if (N == 0) {
    LastValidRow = -1;
    return;
  }
  TRowIterator RowI = BegRI();
  TInt64 count = 1;
  while (count < N) {
    if (!(RowI < EndRI())) {
      return; // The table contains less than N rows
    }
    RowI++;
    count++;
  }
  NumValidRows = N;
  TInt64 LastId = RowI.GetRowIdx();
  if (Next[LastId] == Last) {
    return; // The table contains exactly N rows
  }
  // The table contains more than N rows
  TInt64 CurrId = LastId;
  while (Next[CurrId] != Last) {
    Assert(Next[CurrId] != Invalid);
    TInt64 NextId = Next[CurrId];
    Next[CurrId] = Invalid;
    CurrId = NextId;
  }
  Next[LastId] = Last;
  LastValidRow = LastId;
}

inline void TTable::CheckAndAddIntNode(PNEANet Graph, THashSet<TInt64, int64>& NodeVals, TInt64 NodeId) {
  if (!NodeVals.IsKey(NodeId)) {
    Graph->AddNode(NodeId);
    NodeVals.AddKey(NodeId);
  }
}

inline void TTable::AddEdgeAttributes(PNEANet& Graph, int64 RowId) {
  for (TInt64 i = 0; i < EdgeAttrV.Len(); i++) {
    TStr ColName = EdgeAttrV[i];
    TAttrType T = GetColType(ColName);
    TInt64 Index = GetColIdx(ColName);
    switch (T) {
      case atInt:
        Graph->AddIntAttrDatE(RowId, IntCols[Index][RowId], ColName);
        break;
      case atFlt:
        Graph->AddFltAttrDatE(RowId, FltCols[Index][RowId], ColName);
        break;
      case atStr:
        Graph->AddStrAttrDatE(RowId, GetStrVal(Index, RowId), ColName);
        break;
    }
  }
}

inline void TTable::AddNodeAttributes(TInt64 NId, TStr64V NodeAttrV, TInt64 RowId, THash<TInt64, TStrInt64VH, int64>& NodeIntAttrs,
  THash<TInt64, TStrFlt64VH, int64>& NodeFltAttrs, THash<TInt64, TStrStr64VH, int64>& NodeStrAttrs) {
  for (TInt64 i = 0; i < NodeAttrV.Len(); i++) {
    TStr ColAttr = NodeAttrV[i];
    TAttrType CT = GetColType(ColAttr);
    int64 ColId = GetColIdx(ColAttr);
    // check if this is a common src-dst attribute
    for (TInt64 i = 0; i < CommonNodeAttrs.Len(); i++) {
      if (CommonNodeAttrs[i].Val1 == ColAttr || CommonNodeAttrs[i].Val2 == ColAttr) {
        ColAttr = CommonNodeAttrs[i].Val3;
        break;
      }
    }
    if (CT == atInt) {
      if (!NodeIntAttrs.IsKey(NId)) { NodeIntAttrs.AddKey(NId); }
      if (!NodeIntAttrs.GetDat(NId).IsKey(ColAttr)) { NodeIntAttrs.GetDat(NId).AddKey(ColAttr); }
      NodeIntAttrs.GetDat(NId).GetDat(ColAttr).Add(IntCols[ColId][RowId]);
    } else if (CT == atFlt) {
      if (!NodeFltAttrs.IsKey(NId)) { NodeFltAttrs.AddKey(NId); }
      if (!NodeFltAttrs.GetDat(NId).IsKey(ColAttr)) { NodeFltAttrs.GetDat(NId).AddKey(ColAttr); }
      NodeFltAttrs.GetDat(NId).GetDat(ColAttr).Add(FltCols[ColId][RowId]);
    } else {
      if (!NodeStrAttrs.IsKey(NId)) { NodeStrAttrs.AddKey(NId); }
      if (!NodeStrAttrs.GetDat(NId).IsKey(ColAttr)) { NodeStrAttrs.GetDat(NId).AddKey(ColAttr); }
      NodeStrAttrs.GetDat(NId).GetDat(ColAttr).Add(GetStrVal(ColId, RowId));
    }
  }
}
// Makes one pass over all the rows in the vector RowIds, and builds
// a PNEANet, with each row as an edge between SrcCol and DstCol.
PNEANet TTable::BuildGraph(const TInt64V& RowIds, TAttrAggr AggrPolicy) {
  PNEANet Graph = TNEANet::New();

  const TAttrType NodeType = GetColType(SrcCol);
  Assert(NodeType == GetColType(DstCol));
  const TInt64 SrcColIdx = GetColIdx(SrcCol);
  const TInt64 DstColIdx = GetColIdx(DstCol);

  // node values - i.e. the unique values of src/dst col
  //THashSet<TInt> IntNodeVals; // for both int and string node attr types.
  THash<TFlt, TInt64, int64> FltNodeVals;

  // node attributes
  THash<TInt64, TStrInt64VH, int64> NodeIntAttrs;
  THash<TInt64, TStrFlt64VH, int64> NodeFltAttrs;
  THash<TInt64, TStrStr64VH, int64> NodeStrAttrs;

  // make single pass over all rows in given row id set
  for (TVec<TInt64, int64>::TIter it = RowIds.BegI(); it < RowIds.EndI(); it++) {
    TInt64 CurrRowIdx = *it;

    // add src and dst nodes to graph if they are not seen earlier
    TInt64 SVal, DVal;
    if (NodeType == atFlt) {
      TFlt FSVal = FltCols[SrcColIdx][CurrRowIdx];
      SVal = CheckAndAddFltNode(Graph, FltNodeVals, FSVal);
      TFlt FDVal = FltCols[SrcColIdx][CurrRowIdx];
      DVal = CheckAndAddFltNode(Graph, FltNodeVals, FDVal);
    } else if (NodeType == atInt || NodeType == atStr) {
      if (NodeType == atInt) {
        SVal = IntCols[SrcColIdx][CurrRowIdx];
        DVal = IntCols[DstColIdx][CurrRowIdx];
      } else {
        SVal = StrColMaps[SrcColIdx][CurrRowIdx];
        if (strlen(Context->StringVals.GetKey(SVal)) == 0) { continue; }  //illegal value
        DVal = StrColMaps[DstColIdx][CurrRowIdx];
        if (strlen(Context->StringVals.GetKey(DVal)) == 0) { continue; }  //illegal value
      }
      if (!Graph->IsNode(SVal)) { Graph->AddNode(SVal); }
      if (!Graph->IsNode(DVal)) { Graph->AddNode(DVal); }
      //CheckAndAddIntNode(Graph, IntNodeVals, SVal);
      //CheckAndAddIntNode(Graph, IntNodeVals, DVal);
    }

    // add edge and edge attributes
    Graph->AddEdge(SVal, DVal, CurrRowIdx);
    if (EdgeAttrV.Len() > 0) { AddEdgeAttributes(Graph, CurrRowIdx); }

    // get src and dst node attributes into hashmaps
    if (SrcNodeAttrV.Len() > 0) {
      AddNodeAttributes(SVal, SrcNodeAttrV, CurrRowIdx, NodeIntAttrs, NodeFltAttrs, NodeStrAttrs);
    }
    if (DstNodeAttrV.Len() > 0) {
      AddNodeAttributes(DVal, DstNodeAttrV, CurrRowIdx, NodeIntAttrs, NodeFltAttrs, NodeStrAttrs);
    }
  }

  // aggregate node attributes and add to graph
  if (SrcNodeAttrV.Len() > 0 || DstNodeAttrV.Len() > 0) {
    for (TNEANet::TNodeI NodeI = Graph->BegNI(); NodeI < Graph->EndNI(); NodeI++) {
      TInt64 NId = NodeI.GetId();
      if (NodeIntAttrs.IsKey(NId)) {
        TStrInt64VH IntAttrVals = NodeIntAttrs.GetDat(NId);
        for (TStrInt64VH::TIter it = IntAttrVals.BegI(); it < IntAttrVals.EndI(); it++) {
          TInt64 AttrVal = AggregateVector<TInt64>(it.GetDat(), AggrPolicy);
          Graph->AddIntAttrDatN(NId, AttrVal, it.GetKey());
        }
      }
      if (NodeFltAttrs.IsKey(NId)) {
        TStrFlt64VH FltAttrVals = NodeFltAttrs.GetDat(NId);
        for (TStrFlt64VH::TIter it = FltAttrVals.BegI(); it < FltAttrVals.EndI(); it++) {
          TFlt AttrVal = AggregateVector<TFlt>(it.GetDat(), AggrPolicy);
          Graph->AddFltAttrDatN(NId, AttrVal, it.GetKey());
        }
      }
      if (NodeStrAttrs.IsKey(NId)) {
        TStrStr64VH StrAttrVals = NodeStrAttrs.GetDat(NId);
        for (TStrStr64VH::TIter it = StrAttrVals.BegI(); it < StrAttrVals.EndI(); it++) {
          TStr AttrVal = AggregateVector<TStr>(it.GetDat(), AggrPolicy);
          Graph->AddStrAttrDatN(NId, AttrVal, it.GetKey());
        }
      }
    }
  }

  return Graph;
}

void TTable::InitRowIdBuckets(int64 NumBuckets) {
  for (TInt64 i = 0; i < RowIdBuckets.Len(); i++) {
    RowIdBuckets[i].Clr();
  }
  RowIdBuckets.Clr();

  RowIdBuckets.Gen(NumBuckets);
  for (TInt64 i = 0; i < NumBuckets; i++) {
    RowIdBuckets[i].Gen(10, 0);
  }
}

void TTable::FillBucketsByWindow(TStr SplitAttr, TInt64 JumpSize, TInt64 WindowSize, TInt64 StartVal, TInt64 EndVal) {
  Assert (JumpSize <= WindowSize);
  int64 NumBuckets, MinBucket, MaxBucket;
  TInt64 SplitColId = GetColIdx(SplitAttr);

  if (StartVal == TInt64::Mn || EndVal == TInt64::Mx) {
    // calculate min and max value of the column 'SplitAttr'
    TInt64 MinValue = TInt64::Mx;
    TInt64 MaxValue = TInt64::Mn;
    for (TInt64 i = 0; i < Next.Len(); i++) {
      if (Next[i] != Invalid) {
        if (MinValue > IntCols[SplitColId][i]) {
          MinValue = IntCols[SplitColId][i];
        }
        if (MaxValue < IntCols[SplitColId][i]) {
          MaxValue = IntCols[SplitColId][i];
        }
      }
    }

    if (StartVal == TInt64::Mn) StartVal = MinValue;
    if (EndVal == TInt64::Mx) EndVal = MaxValue;
  }

  // initialize buckets
  NumBuckets = 1;
  if (JumpSize > 0) {
    NumBuckets = (EndVal - StartVal)/JumpSize + 1;
  }

  InitRowIdBuckets(NumBuckets);

  // populate RowIdSets by computing the range of buckets for each row
  for (TInt64 i = 0; i < Next.Len(); i++) {
    if (Next[i] == Invalid) { continue; }
    int64 SplitVal = IntCols[SplitColId][i];
    if (SplitVal < StartVal || SplitVal > EndVal) { continue; }
    int64 RowVal = SplitVal - StartVal;
    if (JumpSize == 0) { // expanding windows
      MinBucket = RowVal/WindowSize;
      MaxBucket = NumBuckets-1;
    } else if (JumpSize == WindowSize) { // disjoint windows
      MinBucket = MaxBucket = RowVal/JumpSize;
    } else { // sliding windows
      if (RowVal < WindowSize) { MinBucket = 0; }
      else { MinBucket = (RowVal-WindowSize)/JumpSize + 1; }
      MaxBucket = RowVal/JumpSize;
    }
    for (TInt64 j = MinBucket; j <= MaxBucket; j++) { RowIdBuckets[j].Add(i); }
  }
}

void TTable::FillBucketsByInterval(TStr SplitAttr, TIntPr64V SplitIntervals) {
  TInt64 SplitColId = GetColIdx(SplitAttr);
  int64 NumBuckets = SplitIntervals.Len();
  InitRowIdBuckets(NumBuckets);

  // populate RowIdSets by computing the range of buckets for each row
  for (TInt64 i = 0; i < Next.Len(); i++) {
    if (Next[i] == Invalid) { continue; }
    int64 SplitVal = IntCols[SplitColId][i];
    for (TInt64 j = 0; j < SplitIntervals.Len(); j++) {
      if (SplitVal >= SplitIntervals[j].Val1 && SplitVal < SplitIntervals[j].Val2) {
        RowIdBuckets[j].Add(i);
      }
    }
  }
}

TVec<PNEANet, int64> TTable::GetGraphsFromSequence(TAttrAggr AggrPolicy) {
  //call BuildGraph on each row id set - parallelizable!
  TVec<PNEANet, int64> GraphSequence;
  for (TInt64 i = 0; i < RowIdBuckets.Len(); i++) {
    if (RowIdBuckets[i].Len() == 0) { continue; }
    PNEANet PNet = BuildGraph(RowIdBuckets[i], AggrPolicy);
    GraphSequence.Add(PNet);
  }

  return GraphSequence;
}

PNEANet TTable::GetFirstGraphFromSequence(TAttrAggr AggrPolicy) {
  CurrBucket = -1;
  this->AggrPolicy = AggrPolicy;
  return GetNextGraphFromSequence();
}

PNEANet TTable::GetNextGraphFromSequence() {
  CurrBucket++;
  while (CurrBucket < RowIdBuckets.Len() && RowIdBuckets[CurrBucket].Len() == 0) {
    CurrBucket++;
  }
  if (CurrBucket >= RowIdBuckets.Len()) { return NULL; }
  return BuildGraph(RowIdBuckets[CurrBucket], AggrPolicy);
}

// Only integer SplitAttr supported
// Setting JumpSize = WindowSize will give disjoint windows
// Setting JumpSize < WindowSize will give sliding windows
// Setting JumpSize > WindowSize will drop certain rows (currently not supported)
// Setting JumpSize = 0 will give expanding windows (i.e. starting at 0 and ending at i*WindowSize)
// To set the range of values of SplitAttr to be considered, use StartVal and EndVal (inclusive)
// If StartVal == TInt.Mn, then the buckets will start from the min value of SplitAttr in the table.
// If EndVal == TInt.Mx, then the buckets will end at the max value of SplitAttr in the table.
TVec<PNEANet, int64> TTable::ToGraphSequence(TStr SplitAttr, TAttrAggr AggrPolicy, TInt64 WindowSize, TInt64 JumpSize, TInt64 StartVal, TInt64 EndVal) {
  FillBucketsByWindow(SplitAttr, JumpSize, WindowSize, StartVal, EndVal);
  printf("buckets filled\n");
  return GetGraphsFromSequence(AggrPolicy);
}

TVec<PNEANet, int64> TTable::ToVarGraphSequence(TStr SplitAttr, TAttrAggr AggrPolicy, TIntPr64V SplitIntervals) {
  FillBucketsByInterval(SplitAttr, SplitIntervals);
  return GetGraphsFromSequence(AggrPolicy);
}

TVec<PNEANet, int64> TTable::ToGraphPerGroup(TStr GroupAttr, TAttrAggr AggrPolicy) {
  return ToGraphSequence(GroupAttr, AggrPolicy, TInt64(1), TInt64(1), TInt64::Mn, TInt64::Mx);
}

PNEANet TTable::ToGraphSequenceIterator(TStr SplitAttr, TAttrAggr AggrPolicy, TInt64 WindowSize, TInt64 JumpSize, TInt64 StartVal, TInt64 EndVal) {
  FillBucketsByWindow(SplitAttr, JumpSize, WindowSize, StartVal, EndVal);
  return GetFirstGraphFromSequence(AggrPolicy);
}

PNEANet TTable::ToVarGraphSequenceIterator(TStr SplitAttr, TAttrAggr AggrPolicy, TIntPr64V SplitIntervals) {
  FillBucketsByInterval(SplitAttr, SplitIntervals);
  return GetFirstGraphFromSequence(AggrPolicy);
}

PNEANet TTable::ToGraphPerGroupIterator(TStr GroupAttr, TAttrAggr AggrPolicy) {
  return ToGraphSequenceIterator(GroupAttr, AggrPolicy, TInt64(1), TInt64(1), TInt64::Mn, TInt64::Mx);
}

// calls to this must be preceded by a call to one of the above ToGraph*Iterator functions
PNEANet TTable::NextGraphIterator() {
  return GetNextGraphFromSequence();
}

TBool TTable::IsLastGraphOfSequence() {
  return CurrBucket >= RowIdBuckets.Len() - 1;
}

PTable TTable::GetNodeTable(const PNEANet& Network, TTableContext* Context) {
  Schema SR;
  SR.Add(TPair<TStr,TAttrType>("node_id",atInt));

  TStr64V IntAttrNames;
  TStr64V FltAttrNames;
  TStr64V StrAttrNames;

  TNEANet::TNodeI NodeI = Network->BegNI();
  NodeI.GetIntAttrNames(IntAttrNames);
  NodeI.GetFltAttrNames(FltAttrNames);
  NodeI.GetStrAttrNames(StrAttrNames);
  for (TInt64 i = 0; i < IntAttrNames.Len(); i++) {
    SR.Add(TPair<TStr,TAttrType>(IntAttrNames[i],atInt));
  }
  for (TInt64 i = 0; i < FltAttrNames.Len(); i++) {
    SR.Add(TPair<TStr,TAttrType>(FltAttrNames[i],atFlt));
  }
  for (TInt64 i = 0; i < StrAttrNames.Len(); i++) {
    SR.Add(TPair<TStr,TAttrType>(StrAttrNames[i],atStr));
  }

  PTable T = New(SR, Context);

  TInt64 Cnt = 0;
  // populate table columns
  while (NodeI < Network->EndNI()) {
    T->IntCols[0].Add(NodeI.GetId());
    for (TInt64 i = 0; i < IntAttrNames.Len(); i++) {
      T->IntCols[i+1].Add(Network->GetIntAttrDatN(NodeI,IntAttrNames[i]));
    }
    for (TInt64 i = 0; i < FltAttrNames.Len(); i++) {
      T->FltCols[i].Add(Network->GetFltAttrDatN(NodeI,FltAttrNames[i]));
    }
    for (TInt64 i = 0; i < StrAttrNames.Len(); i++) {
      T->AddStrVal(i, Network->GetStrAttrDatN(NodeI,StrAttrNames[i]));
    }
    Cnt++;
    NodeI++;
  }
  // set number of rows and "Next" vector
  T->NumRows = Cnt;
  T->NumValidRows = T->NumRows;
  T->Next = TInt64V(T->NumRows,0);
  for (TInt64 i = 0; i < T->NumRows-1; i++) {
    T->Next.Add(i+1);
  }
  T->LastValidRow = T->NumRows-1;
  T->Next.Add(Last);
  return T;
}

PTable TTable::GetEdgeTable(const PNEANet& Network, TTableContext* Context) {
  Schema SR;
  SR.Add(TPair<TStr,TAttrType>("edg_id",atInt));
  SR.Add(TPair<TStr,TAttrType>("src_id",atInt));
  SR.Add(TPair<TStr,TAttrType>("dst_id",atInt));

  TStr64V IntAttrNames;
  TStr64V FltAttrNames;
  TStr64V StrAttrNames;

  TNEANet::TEdgeI EdgeI = Network->BegEI();
  EdgeI.GetIntAttrNames(IntAttrNames);
  EdgeI.GetFltAttrNames(FltAttrNames);
  EdgeI.GetStrAttrNames(StrAttrNames);
  for (TInt64 i = 0; i < IntAttrNames.Len(); i++) {
    SR.Add(TPair<TStr,TAttrType>(IntAttrNames[i],atInt));
  }
  for (TInt64 i = 0; i < FltAttrNames.Len(); i++) {
    SR.Add(TPair<TStr,TAttrType>(FltAttrNames[i],atFlt));
  }
  for (TInt64 i = 0; i < StrAttrNames.Len(); i++) {
    //printf("%s\n",StrAttrNames[i].CStr());
    SR.Add(TPair<TStr,TAttrType>(StrAttrNames[i],atStr));
  }

  PTable T = New(SR, Context);

  TInt64 Cnt = 0;
  // populate table columns
  while (EdgeI < Network->EndEI()) {
    T->IntCols[0].Add(EdgeI.GetId());
    T->IntCols[1].Add(EdgeI.GetSrcNId());
    T->IntCols[2].Add(EdgeI.GetDstNId());
    for (TInt64 i = 0; i < IntAttrNames.Len(); i++) {
      T->IntCols[i+3].Add(Network->GetIntAttrDatE(EdgeI,IntAttrNames[i]));
    }
    for (TInt64 i = 0; i < FltAttrNames.Len(); i++) {
      T->FltCols[i].Add(Network->GetFltAttrDatE(EdgeI,FltAttrNames[i]));
    }
    for (TInt64 i = 0; i < StrAttrNames.Len(); i++) {
      T->AddStrVal(i, Network->GetStrAttrDatE(EdgeI,StrAttrNames[i]));
    }
    Cnt++;
    EdgeI++;
  }
  // set number of rows and "Next" vector
  T->NumRows = Cnt;
  T->NumValidRows = T->NumRows;
  T->Next = TInt64V(T->NumRows,0);
  for (TInt64 i = 0; i < T->NumRows-1; i++) {
    T->Next.Add(i+1);
  }
  T->LastValidRow = T->NumRows-1;
  T->Next.Add(Last);
  return T;
}
#ifdef GCC_ATOMIC


PTable TTable::GetEdgeTablePN(const PNGraphMP& Network, TTableContext* Context){
  Schema SR;
  SR.Add(TPair<TStr,TAttrType>("src_id",atInt));
  SR.Add(TPair<TStr,TAttrType>("dst_id",atInt));

  TNGraphMP::TEdgeI FirstEI = Network->BegEI();
  PTable T = New(SR, Context);
  TInt64 NumEdges = Network->GetEdges();
  TInt64 NumPartitions = omp_get_max_threads()*CHUNKS_PER_THREAD;
  TInt64 PartitionSize = NumEdges/NumPartitions;
  if (PartitionSize*NumPartitions < NumEdges) { NumPartitions++;}

  typedef TPair<TNGraphMP::TEdgeI, TNGraphMP::TEdgeI> TEIPr;
  TVec<TEIPr, int64> Partitions;
  TInt64V PartitionSizes;
  TNGraphMP::TEdgeI currStart = FirstEI;
  TInt64 currCount = 0;
  while (FirstEI < Network->EndEI()){
    if (currCount == PartitionSize) {
      Partitions.Add(TEIPr(currStart, FirstEI));
      currStart = FirstEI;
      PartitionSizes.Add(currCount);
      //printf("added: %d\n", currCount.Val);
      currCount = 0;
    }
    //printf("%d\n", currCount.Val);
    FirstEI++;
    currCount++;
  }
  Partitions.Add(TEIPr(currStart, FirstEI));
  PartitionSizes.Add(currCount);

  T->ResizeTable(NumEdges);
  #pragma omp parallel for schedule(dynamic, CHUNKS_PER_THREAD)
  for (int64 p = 0; p < Partitions.Len(); p++) {
    TNGraphMP::TEdgeI EdgeI = Partitions[p].GetVal1();
    TNGraphMP::TEdgeI EndI = Partitions[p].GetVal2();
    //printf("Thread = %d, p = %d, size = %d\n", omp_get_thread_num(), p, PartitionSizes[p].Val);
    int64 start = T->GetEmptyRowsStart(PartitionSizes[p]);
    while (EdgeI < EndI) {
      T->IntCols[0][start] = EdgeI.GetSrcNId();
      T->IntCols[1][start] = EdgeI.GetDstNId();
      EdgeI++;
      if (EdgeI < EndI) { T->Next[start] = start+1;}
      start++;
    }
  }

  Assert(T->NumRows == NumEdges);
  return T;
}

#endif // GCC_ATOMIC
PTable TTable::GetFltNodePropertyTable(const PNEANet& Network, const TIntFlt64H& Property,
 const TStr& NodeAttrName, const TAttrType& NodeAttrType, const TStr& PropertyAttrName,
 TTableContext* Context) {
  Schema SR;
  // Determine type of node id
  SR.Add(TPair<TStr,TAttrType>(NodeAttrName,NodeAttrType));
  SR.Add(TPair<TStr,TAttrType>(PropertyAttrName,atFlt));
  PTable T = New(SR, Context);
  TInt64 NodeColIdx = T->GetColIdx(NodeAttrName);
  TInt64 Cnt = 0;
  // populate table columns
  for (TNEANet::TNodeI NodeI = Network->BegNI(); NodeI < Network->EndNI(); NodeI++) {
    switch (NodeAttrType) {
      case atInt:
        T->IntCols[NodeColIdx].Add(Network->GetIntAttrDatN(NodeI,NodeAttrName));
        break;
      case atFlt:
        T->FltCols[NodeColIdx].Add(Network->GetFltAttrDatN(NodeI,NodeAttrName));
        break;
      case atStr:
        T->AddStrVal(TInt64(0), Network->GetStrAttrDatN(NodeI,NodeAttrName));
        break;
    }
    T->FltCols[0].Add(Property.GetDat(NodeI.GetId()));
    Cnt++;
  }
  // set number of rows and "Next" vector
  T->NumRows = Cnt;
  T->NumValidRows = T->NumRows;
  T->Next = TInt64V(T->NumRows,0);
  for (TInt64 i = 0; i < T->NumRows-1; i++) {
    T->Next.Add(i+1);
  }
  T->LastValidRow = T->NumRows-1;
  T->Next.Add(Last);
  return T;
}
/*** Special Filters ***/
PTable TTable::IsNextK(const TStr& OrderCol, TInt64 K, const TStr& GroupBy, const TStr& RankColName) {
  TStr64V OrderBy;
  if (GroupBy.Empty()) {
    OrderBy.Add(OrderCol);
  } else {
    OrderBy.Add(GroupBy);
    OrderBy.Add(OrderCol);
  }
  if (RankColName.Empty()) {
    Order(OrderBy);
  } else {
    Order(OrderBy, RankColName, true);
  }
  TAttrType GroupByAttrType = GetColType(GroupBy);
  PTable T = InitializeJointTable(*this);
  for (TRowIterator RI = BegRI(); RI < EndRI(); RI++) {
    TInt64 Succ = RI.GetRowIdx();
    TBool OutOfGroup = false;
    for (TInt64 i = 0; i < K; i++) {
      Succ = Next[Succ];
      if (Succ == Last) { break; }
      switch (GroupByAttrType) {
        case atInt:
          if (GetIntVal(GroupBy, Succ) != RI.GetIntAttr(GroupBy)) { OutOfGroup = true; }
          break;
        case atFlt:
          if (GetFltVal(GroupBy, Succ) != RI.GetFltAttr(GroupBy)) { OutOfGroup = true; }
          break;
        case atStr:
          if (GetStrVal(GroupBy, Succ) != RI.GetStrAttr(GroupBy)) { OutOfGroup = true; }
          break;
      }
      if (OutOfGroup) { break; }  // break out of inner for loop
      T->AddJointRow(*this, *this, RI.GetRowIdx(), Succ);
    }
  }
  return T;
}

void TTable::PrintSize(){
  printf("Total number of rows: %s\n", TInt64::GetStr(NumRows.Val).CStr());
  printf("Number of valid rows: %s\n", TInt64::GetStr(NumValidRows.Val).CStr());
  printf("Number of Int columns: %s\n", TInt64::GetStr(IntCols.Len()).CStr());
  printf("Number of Flt columns: %s\n", TInt64::GetStr(FltCols.Len()).CStr());
  printf("Number of Str columns: %s\n", TInt64::GetStr(StrColMaps.Len()).CStr());
  TSize MemUsed = GetMemUsedKB();
  printf("Approximate table size is %s KB\n", TUInt64::GetStr(MemUsed).CStr());
}

TSize TTable::GetMemUsedKB() {
  TSize ApproxSize = 0;
  ApproxSize += Next.GetMemUsed()/1000;  // Next vector
  for(int64 i = 0; i < IntCols.Len(); i++){
    ApproxSize += IntCols[i].GetMemUsed()/1000;
  }
  for(int64 i = 0; i < FltCols.Len(); i++){
    ApproxSize += FltCols[i].GetMemUsed()/1000;
  }
  for(int64 i = 0; i < StrColMaps.Len(); i++){
    ApproxSize += StrColMaps[i].GetMemUsed()/1000;
  }
  ApproxSize += RowIdMap.GetMemUsed()/1000;
  ApproxSize += GroupIDMapping.GetMemUsed()/1000;
  ApproxSize += GroupMapping.GetMemUsed()/1000;
  ApproxSize += RowIdBuckets.GetMemUsed() / 1000;
  return ApproxSize;
}

void TTable::PrintContextSize(){
  printf("Number of strings in pool: ");
  printf("%s\n", TInt64::GetStr(Context->StringVals.Len()).CStr());
  printf("Number of entries in hash table: ");
  printf("%s\n", TInt64::GetStr(Context->StringVals.Reserved()).CStr());
  TSize MemUsed = GetContextMemUsedKB();
  printf("Approximate context size is %s KB\n",
          TUInt64::GetStr(MemUsed).CStr());
}

TSize TTable::GetContextMemUsedKB(){
  TSize ApproxSize = 0;
  ApproxSize += Context->StringVals.GetMemUsed();
  return ApproxSize;
}

void TTable::AddTable(const TTable& T) {
  //for (TInt c = 0; c < S.Len(); c++) {
  //  if (S[c] != T.S[c]) { printf("(%s,%d) != (%s,%d)\n", S[c].Val1.CStr(), S[c].Val2, T.S[c].Val1.CStr(), T.S[c].Val2); TExcept::Throw("when adding tables, their schemas must match!"); }
  //}
  for (TInt64 c = 0; c < Sch.Len(); c++) {
    TStr ColName = GetSchemaColName(c);
    TInt64 ColIdx = GetColIdx(ColName);
    TInt64 TColIdx = ColName == IdColName ? T.GetColIdx(T.IdColName) : T.GetColIdx(ColName);
    if (TColIdx < 0) { TExcept::Throw("when adding a table, it must contain all columns of source table!"); }
    switch (GetColType(ColName)) {
    case atInt:
       IntCols[ColIdx].AddV(T.IntCols[TColIdx]);
       break;
    case atFlt:
       FltCols[ColIdx].AddV(T.FltCols[TColIdx]);
       break;
    case atStr:
       StrColMaps[ColIdx].AddV(T.StrColMaps[TColIdx]);
       break;
    }
  }

  TInt64V TNext(T.Next);
  for (TInt64 i = 0; i < TNext.Len(); i++) {
    if (TNext[i] != Last && TNext[i] != Invalid) { TNext[i] += NumRows; }
  }
  //BUG FIX
  if (Next.Len() == 0) {
    FirstValidRow = T.FirstValidRow;
  }
  Next.AddV(TNext);
  // checks if table is empty
  if (LastValidRow >= 0) {
    Next[LastValidRow] = NumRows + T.FirstValidRow;
  }
  LastValidRow = NumRows + T.LastValidRow;
  NumRows += T.NumRows;
  NumValidRows += T.NumValidRows;
}

// returns physical indices of rows of given table present in our table
// we assume that schema matches exactly (including index of id cols)
void TTable::GetCollidingRows(const TTable& Table, THashSet<TInt64, int64>& Collisions) {
  TInt64V UniqueVec;
  THash<TGroupKey, TPair<TInt64, TInt64V>, int64>Grouping;
  TStr64V GroupBy;

  // indices of columns of each type
  TInt64V IntGroupByCols;
  TInt64V FltGroupByCols;
  TInt64V StrGroupByCols;

  TInt64 IKLen, FKLen, SKLen;

  // check that schemas match
  for (TInt64 c = 0; c < Sch.Len(); c++) {
    if (Sch[c].Val1 == IdColName) {
      if (Table.Sch[c].Val1 != Table.GetIdColName()) {
        TExcept::Throw("GetCollidingRows: schemas do not match!");
      }
      continue;
    }
    if (Sch[c] != Table.Sch[c]) {
      printf("(%s,%d) != (%s,%d)\n", Sch[c].Val1.CStr(), Sch[c].Val2, Table.Sch[c].Val1.CStr(), Table.Sch[c].Val2);
      TExcept::Throw("GetCollidingRows: schemas do not match!");
    }
    GroupBy.Add(NormalizeColName(Sch[c].Val1));
    TPair<TAttrType, TInt64> ColType = Table.GetColTypeMap(Sch[c].Val1);
    switch (ColType.Val1) {
      case atInt:
        IntGroupByCols.Add(ColType.Val2);
        break;
      case atFlt:
        FltGroupByCols.Add(ColType.Val2);
        break;
      case atStr:
        StrGroupByCols.Add(ColType.Val2);
        break;
    }
  }

  IKLen = IntGroupByCols.Len();
  FKLen = FltGroupByCols.Len();
  SKLen = StrGroupByCols.Len();

  // group rows of first table
  GroupAux(GroupBy, Grouping, true, "", false, UniqueVec, true);

  // find colliding rows of second table
  for (TRowIterator it = Table.BegRI(); it < Table.EndRI(); it++) {
    // read keys from row
    TInt64V IKey(IKLen + SKLen, 0);
    TFlt64V FKey(FKLen, 0);

    // find group key
    for (TInt64 c = 0; c < IKLen; c++) {
      IKey.Add(it.GetIntAttr(IntGroupByCols[c]));
    }
    for (TInt64 c = 0; c < FKLen; c++) {
      FKey.Add(it.GetFltAttr(FltGroupByCols[c]));
    }
    for (TInt64 c = 0; c < SKLen; c++) {
      IKey.Add(it.GetStrMapById(StrGroupByCols[c]));
    }
    // look for group matching the key
    TGroupKey GroupKey = TGroupKey(IKey, FKey);

    TInt64 RowIdx = it.GetRowIdx();
    if (Grouping.IsKey(GroupKey)) {
      // row exists in first table
      Collisions.AddKey(RowIdx);
    }
  }
}

void TTable::StoreIntCol(const TStr& ColName, const TInt64V& ColVals) {
  if (ColVals.Len() != NumRows) {
    printf("new column dimension must agree with number of rows\n");
    return;
  }
  AddSchemaCol(ColName, atInt);
  IntCols.Add(TInt64V(NumRows));
  TInt64 ColIdx = IntCols.Len()-1;
  TInt64 i = 0;
  for (TRowIterator RI = BegRI(); RI < EndRI(); RI++) {
    IntCols[ColIdx][RI.GetRowIdx()] = ColVals[i];
    i++;
  }
  TInt64 L = IntCols.Len();
  AddColType(ColName, atInt, L-1);
}

void TTable::StoreFltCol(const TStr& ColName, const TFlt64V& ColVals) {
  if (ColVals.Len() != NumRows) {
    printf("new column dimension must agree with number of rows\n");
    return;
  }
  AddSchemaCol(ColName, atFlt);
  FltCols.Add(TFlt64V(NumRows));
  TInt64 ColIdx = FltCols.Len()-1;
  TInt64 i = 0;
  for (TRowIterator RI = BegRI(); RI < EndRI(); RI++) {
    FltCols[ColIdx][RI.GetRowIdx()] = ColVals[i];
    i++;
  }
  TInt64 L = FltCols.Len();
  AddColType(ColName, atFlt, L-1);
}

void TTable::StoreStrCol(const TStr& ColName, const TStr64V& ColVals) {
  if (ColVals.Len() != NumRows) {
    printf("new column dimension must agree with number of rows\n");
    return;
  }
  AddSchemaCol(ColName, atStr);
  StrColMaps.Add(TInt64V(NumRows,0));
  TInt64 ColIdx = FltCols.Len()-1;
  TInt64 i = 0;
  for (TRowIterator RI = BegRI(); RI < EndRI(); RI++) {
    TInt64 Key = Context->StringVals.GetKeyId(ColVals[i]);
    if (Key == -1) { Context->StringVals.AddKey(ColVals[i]); }
    StrColMaps[ColIdx][RI.GetRowIdx()] = Key;
    i++;
  }
  TInt64 L = StrColMaps.Len();
  AddColType(ColName, atStr, L-1);
}

void TTable::UpdateTableForNewRow() {
  if (LastValidRow >= 0) {
    Next[LastValidRow] = NumRows;
  }
  Next.Add(Last);
  LastValidRow = NumRows;

  NumRows++;
  NumValidRows++;
}

#ifdef GCC_ATOMIC
void TTable::SetFltColToConstMP(TInt64 UpdateColIdx, TFlt DefaultFltVal){
    if(!GetMP()){ TExcept::Throw("Not Using MP!");}
  TIntPr64V Partitions;
  GetPartitionRanges(Partitions, omp_get_max_threads()*CHUNKS_PER_THREAD);
  //TInt64 PartitionSize = Partitions[0].GetVal2()-Partitions[0].GetVal1()+1;
  #pragma omp parallel for schedule(dynamic, CHUNKS_PER_THREAD)
  for (int64 i = 0; i < Partitions.Len(); i++){
    TRowIterator RowI(Partitions[i].GetVal1(), this);
    TRowIterator EndI(Partitions[i].GetVal2(), this);
    while(RowI < EndI){
      FltCols[UpdateColIdx][RowI.GetRowIdx()] = DefaultFltVal;
      RowI++;
    }
  }
}

// OP RS 2016/06/30: this wrapper function is required
//   for the code to compile on Mac OS X gcc 4.2.1
int64 sync_bool_compare_and_swap(int64 *lock) {
  return(__sync_bool_compare_and_swap(lock, 0, 1));
}

void TTable::UpdateFltFromTableMP(const TStr& KeyAttr, const TStr& UpdateAttr,
    const TTable& Table, const TStr& FKeyAttr, const TStr& ReadAttr,
    TFlt DefaultFltVal) {
  if (!GetMP()) {
    TExcept::Throw("Not Using MP!");
  }

  TAttrType KeyType = GetColType(KeyAttr);
  TAttrType FKeyType = Table.GetColType(FKeyAttr);
  if(KeyType != FKeyType){TExcept::Throw("Key Type Mismatch");}
  if(GetColType(UpdateAttr) != atFlt || Table.GetColType(ReadAttr) != atFlt){
    TExcept::Throw("Expecting Float values");
  }
  TStr NKeyAttr = NormalizeColName(KeyAttr);
  //TStr NUpdateAttr = NormalizeColName(UpdateAttr);
  //TStr NFKeyAttr = Table.NormalizeColName(FKeyAttr);
  //TStr NReadAttr = Table.NormalizeColName(ReadAttr);
  TInt64 UpdateColIdx = GetColIdx(UpdateAttr);
  TInt64 FKeyColIdx = GetColIdx(FKeyAttr);
  TInt64 ReadColIdx = GetColIdx(ReadAttr);

  // TODO: this should be a generic vector operation
  SetFltColToConstMP(UpdateColIdx, DefaultFltVal);

  TIntPr64V Partitions;
  Table.GetPartitionRanges(Partitions, omp_get_max_threads()*CHUNKS_PER_THREAD);
  //TInt64 PartitionSize = Partitions[0].GetVal2()-Partitions[0].GetVal1()+1;
  TInt64V Locks(NumRows);
  Locks.PutAll(0);  // need to parallelize this...

  switch (KeyType) {
    // TODO: add support for other cases of KeyType
    case atInt: {
        THashMP<TInt64,TInt64V,int64> Grouping;
        // must use physical row ids
        GroupByIntColMP(NKeyAttr, Grouping, true);
        #pragma omp parallel for schedule(dynamic, CHUNKS_PER_THREAD) // num_threads(1)
        for (int64 i = 0; i < Partitions.Len(); i++) {
          TRowIterator RowI(Partitions[i].GetVal1(), &Table);
          TRowIterator EndI(Partitions[i].GetVal2(), &Table);
          while (RowI < EndI) {
            TInt64 K = RowI.GetIntAttr(FKeyColIdx);
            if (Grouping.IsKey(K)) {
              TInt64V& UpdateRows = Grouping.GetDat(K);
              for (int64 j = 0; j < UpdateRows.Len(); j++) {
                int64* lock = &Locks[UpdateRows[j]].Val;
                // OP RS 2016/06/30: needed to define a wrapper function
                //   for the code to compile on Mac OS X gcc 4.2.1
                //if (!__sync_bool_compare_and_swap(lock, 0, 1)) {
                if (!sync_bool_compare_and_swap(lock)) {
                  continue;
                }
                //printf("key = %d, row = %d, old_score = %f\n", K.Val, j, UpdateRows[j].Val, FltCols[UpdateColIdx][UpdateRows[j]].Val);
                  FltCols[UpdateColIdx][UpdateRows[j]] = RowI.GetFltAttr(ReadColIdx);
                  //printf("key = %d, new_score = %f\n", K.Val, j, FltCols[UpdateColIdx][UpdateRows[j]].Val);
              } // end of for loop
            } // end of if statement
            RowI++;
          } // end of while loop
        }  // end of for loop
      } // end of case atInt
      break;
    default:
      break;
  } // end of outer switch statement
}
#endif  // GCC_ATOMIC

void TTable::UpdateFltFromTable(const TStr& KeyAttr, const TStr& UpdateAttr, const TTable& Table,
  const TStr& FKeyAttr, const TStr& ReadAttr, TFlt DefaultFltVal){
  if(!IsColName(KeyAttr)){ TExcept::Throw("Bad KeyAttr parameter");}
  if(!IsColName(UpdateAttr)){ TExcept::Throw("Bad UpdateAttr parameter");}
  if(!Table.IsColName(FKeyAttr)){ TExcept::Throw("Bad FKeyAttr parameter");}
  if(!Table.IsColName(ReadAttr)){ TExcept::Throw("Bad ReadAttr parameter");}

#ifdef GCC_ATOMIC
  if(GetMP()){
    UpdateFltFromTableMP(KeyAttr, UpdateAttr,Table, FKeyAttr, ReadAttr, DefaultFltVal);
    return;
  }
#endif  // GCC_ATOMIC

  TAttrType KeyType = GetColType(KeyAttr);
  TAttrType FKeyType = Table.GetColType(FKeyAttr);
  if(KeyType != FKeyType){TExcept::Throw("Key Type Mismatch");}
  if(GetColType(UpdateAttr) != atFlt || Table.GetColType(ReadAttr) != atFlt){
    TExcept::Throw("Expecting Float values");
  }
  TStr NKeyAttr = NormalizeColName(KeyAttr);
  TStr NUpdateAttr = NormalizeColName(UpdateAttr);
  TStr NFKeyAttr = Table.NormalizeColName(FKeyAttr);
  TStr NReadAttr = Table.NormalizeColName(ReadAttr);
  TInt64 UpdateColIdx = GetColIdx(UpdateAttr);

  for(TRowIterator iter = BegRI(); iter < EndRI(); iter++){
    FltCols[UpdateColIdx][iter.GetRowIdx()] = DefaultFltVal;
  }

  switch(KeyType) {
    // TODO: add support for other cases of KeyType
    case atInt: {
        TIntInt64V64H Grouping;
        GroupByIntCol(NKeyAttr, Grouping, TInt64V(), true, true);
        for (TRowIterator RI = Table.BegRI(); RI < Table.EndRI(); RI++) {
          TInt64 K = RI.GetIntAttr(NFKeyAttr);
          if (Grouping.IsKey(K)) {
            TInt64V& UpdateRows = Grouping.GetDat(K);
            for (int64 i = 0; i < UpdateRows.Len(); i++) {
              FltCols[UpdateColIdx][UpdateRows[i]] = RI.GetFltAttr(NReadAttr);
            } // end of for loop
          } // end of if statement
        } // end of for loop
      } // end of case atInt
      break;
    default:
      break;
  } // end of outer switch statement
}


// can ONLY be called when a table is being initialised (before IDs are allocated)
void TTable::AddRow(const TRowIterator& RI) {
  for (TInt64 c = 0; c < Sch.Len(); c++) {
    TStr ColName = GetSchemaColName(c);
    if (ColName == IdColName) { continue; }

    TInt64 ColIdx = GetColIdx(ColName);

    switch (GetColType(ColName)) {
    case atInt:
       IntCols[ColIdx].Add(RI.GetIntAttr(ColName));
       break;
    case atFlt:
       FltCols[ColIdx].Add(RI.GetFltAttr(ColName));
       break;
    case atStr:
       StrColMaps[ColIdx].Add(RI.GetStrMapByName(ColName));
       break;
    }
  }
  UpdateTableForNewRow();
}

void TTable::AddRow(const TInt64V& IntVals, const TFlt64V& FltVals, const TStr64V& StrVals) {
  for (TInt64 c = 0; c < IntVals.Len(); c++) {
    IntCols[c].Add(IntVals[c]);
  }
  for (TInt64 c = 0; c < FltVals.Len(); c++) {
    FltCols[c].Add(FltVals[c]);
  }
  for (TInt64 c = 0; c < StrVals.Len(); c++) {
    AddStrVal(c, StrVals[c]);
  }
  UpdateTableForNewRow();
}

void TTable::ResizeTable(int64 RowCount) {
  if (RowCount == 0) {
    // initialize empty table
    NumValidRows = 0;
    FirstValidRow = TTable::Invalid;
    LastValidRow = TTable::Invalid;
  }
  if (Next.Len() < RowCount) {
    TInt64 FltOffset = IntCols.Len();
    TInt64 StrOffset = FltOffset + FltCols.Len();
    TInt64 TotalCols = StrOffset + StrColMaps.Len();
#ifdef USE_OPENMP
    #pragma omp parallel for schedule(static)
#endif
    for (int64 i = 0; i < TotalCols+1; i++) {
      if (i < FltOffset) {
        IntCols[i].Reserve(RowCount, RowCount);
      } else if (i < StrOffset) {
        FltCols[i-FltOffset].Reserve(RowCount, RowCount);
      } else if (i < TotalCols) {
        StrColMaps[i-StrOffset].Reserve(RowCount, RowCount);
      } else {
        Next.Reserve(RowCount, RowCount);
      }
    }
  } else if (Next.Len() > RowCount) {
    TInt64 FltOffset = IntCols.Len();
    TInt64 StrOffset = FltOffset + FltCols.Len();
    TInt64 TotalCols = StrOffset + StrColMaps.Len();
#ifdef USE_OPENMP
    #pragma omp parallel for schedule(static)
#endif
    for (int64 i = 0; i < TotalCols+1; i++) {
      if (i < FltOffset) {
        IntCols[i].Trunc(RowCount);
      } else if (i < StrOffset) {
        FltCols[i-FltOffset].Trunc(RowCount);
      } else if (i < TotalCols) {
        StrColMaps[i-StrOffset].Trunc(RowCount);
      } else {
        Next.Trunc(RowCount);
      }
    }
  }
}

int64 TTable::GetEmptyRowsStart(int64 NewRows) {
  int64 start = -1;
#ifdef USE_OPENMP
  #pragma omp critical
  {
#endif
    start = NumRows;
    NumRows += NewRows;
    NumValidRows += NewRows;
    // To make this function thread-safe, the following call must be done before the
    // code enters parallel region.
    // ResizeTable(NumRows);
    Assert(NumRows <= Next.Len());
    if (LastValidRow >= 0) {Next[LastValidRow] = start;}
    LastValidRow = start+NewRows-1;
    Next[LastValidRow] = Last;
#ifdef USE_OPENMP
  }
#endif
  Assert (start >= 0);
  return start;
}

void TTable::AddSelectedRows(const TTable& Table, const TInt64V& RowIDs) {
  int64 NewRows = RowIDs.Len();
  if (NewRows == 0) { return; }
  // this call should be thread-safe
  int64 start = GetEmptyRowsStart(NewRows);
  for (TInt64 r = 0; r < NewRows; r++) {
    TInt64 CurrRowIdx = RowIDs[r];
    for (TInt64 i = 0; i < Table.IntCols.Len(); i++) {
      IntCols[i][start+r] = Table.IntCols[i][CurrRowIdx];
    }
    for (TInt64 i = 0; i < Table.FltCols.Len(); i++) {
      FltCols[i][start+r] = Table.FltCols[i][CurrRowIdx];
    }
    for (TInt64 i = 0; i < Table.StrColMaps.Len(); i++) {
      StrColMaps[i][start+r] = Table.StrColMaps[i][CurrRowIdx];
    }
  }
  for (TInt64 r = 0; r < NewRows-1; r++) {
    Next[start+r] = start+r+1;
  }
}

void TTable::AddNRows(int64 NewRows, const TVec<TInt64V, int64>& IntColsP, const TVec<TFlt64V, int64>& FltColsP, const TVec<TInt64V, int64>& StrColMapsP) {
  if (NewRows == 0) { return; }
  // this call should be thread-safe
  int64 start = GetEmptyRowsStart(NewRows);
  for (TInt64 r = 0; r < NewRows; r++) {
    for (TInt64 i = 0; i < IntColsP.Len(); i++) {
      IntCols[i][start+r] = IntColsP[i][r];
    }
    for (TInt64 i = 0; i < FltColsP.Len(); i++) {
      FltCols[i][start+r] = FltColsP[i][r];
    }
    for (TInt64 i = 0; i < StrColMapsP.Len(); i++) {
      StrColMaps[i][start+r] = StrColMapsP[i][r];
    }
  }
  for (TInt64 r = 0; r < NewRows-1; r++) {
    Next[start+r] = start+r+1;
  }
}

#ifdef USE_OPENMP
void TTable::AddNJointRowsMP(const TTable& T1, const TTable& T2, const TVec<TIntPr64V, int64>& JointRowIDSet) {
  //double startFn = omp_get_wtime();
  int64 JointTableSize = 0;
  TInt64V StartOffsets(JointRowIDSet.Len());
  for (int64 i = 0; i < JointRowIDSet.Len(); i++) {
    StartOffsets[i] = JointTableSize;
    JointTableSize += JointRowIDSet[i].Len();
  }
  if (JointTableSize == 0) {
    TExcept::Throw("Joint table is empty");
  }
  //double endOffsets = omp_get_wtime();
  //printf("Offsets time = %f\n",endOffsets-startFn);
  ResizeTable(JointTableSize);
  //double endResize = omp_get_wtime();
  //printf("Resize time = %f\n",endResize-endOffsets);
  NumRows = JointTableSize;
  NumValidRows = JointTableSize;
  Assert(NumRows <= Next.Len());

  TInt64 IntOffset = T1.IntCols.Len();
  TInt64 FltOffset = T1.FltCols.Len();
  TInt64 StrOffset = T1.StrColMaps.Len();

  TInt64 IdOffset = IntOffset + T2.IntCols.Len();
  RowIdMap.Clr();
  for (TInt64 IdCnt = 0; IdCnt < JointTableSize; IdCnt++) {
    RowIdMap.AddDat(IdCnt, IdCnt);
  }

  #pragma omp parallel for schedule(dynamic, CHUNKS_PER_THREAD)
  for (int64 j = 0; j < JointRowIDSet.Len(); j++) {
    const TIntPr64V& RowIDs = JointRowIDSet[j];
    int64 start = StartOffsets[j];
    int64 NewRows = RowIDs.Len();
    if (NewRows == 0) {continue;}
    for (TInt64 r = 0; r < NewRows; r++){
      TInt64Pr CurrRowIdPr = RowIDs[r];
      for(TInt64 i = 0; i < T1.IntCols.Len(); i++){
        IntCols[i][start+r] = T1.IntCols[i][CurrRowIdPr.GetVal1()];
      }
      for(TInt64 i = 0; i < T1.FltCols.Len(); i++){
        FltCols[i][start+r] = T1.FltCols[i][CurrRowIdPr.GetVal1()];
      }
      for(TInt64 i = 0; i < T1.StrColMaps.Len(); i++){
        StrColMaps[i][start+r] = T1.StrColMaps[i][CurrRowIdPr.GetVal1()];
      }
      for(TInt64 i = 0; i < T2.IntCols.Len(); i++){
        IntCols[i+IntOffset][start+r] = T2.IntCols[i][CurrRowIdPr.GetVal2()];
      }
      for(TInt64 i = 0; i < T2.FltCols.Len(); i++){
        FltCols[i+FltOffset][start+r] = T2.FltCols[i][CurrRowIdPr.GetVal2()];
      }
      for(TInt64 i = 0; i < T2.StrColMaps.Len(); i++){
        StrColMaps[i+StrOffset][start+r] = T2.StrColMaps[i][CurrRowIdPr.GetVal2()];
      }
      IntCols[IdOffset][start+r] = start+r;
    }
    for(TInt64 r = 0; r < NewRows; r++){
      Next[start+r] = start+r+1;
    }
  }
  LastValidRow = JointTableSize-1;
  Next[LastValidRow] = Last;
  //double endIterate = omp_get_wtime();
  //printf("Iterate time = %f\n",endIterate-endResize);
}
#endif // USE_OPENMP

PTable TTable::UnionAll(const TTable& Table) {
  Schema NewSchema;
  for (TInt64 c = 0; c < Sch.Len(); c++) {
    if (Sch[c].Val1 != GetIdColName()) {
      NewSchema.Add(TPair<TStr,TAttrType>(Sch[c].Val1, Sch[c].Val2));
    }
  }
  PTable result = TTable::New(NewSchema, Context);
  result->AddTable(*this);
  result->UnionAllInPlace(Table);
  return result;
}

void TTable::UnionAllInPlace(const TTable& Table) {
  AddTable(Table);
  // TODO: For the moment, IDs are not initialized (to avoid having too many ID columns)
  //result->InitIds();
}


PTable TTable::Union(const TTable& Table) {
  Schema NewSchema;
  THashSet<TInt64, int64> Collisions;
  TStr64V ColNames;

  for (TInt64 c = 0; c < Sch.Len(); c++) {
    if (Sch[c].Val1 != GetIdColName()) {
      NewSchema.Add(TPair<TStr,TAttrType>(Sch[c].Val1, Sch[c].Val2));
      ColNames.Add(Sch[c].Val1);
    }
  }
  PTable result = TTable::New(NewSchema, Context);

  GetCollidingRows(Table, Collisions);

  result->AddTable(*this);

  result->Unique(ColNames);

  // this part should be made faster by adding all the rows in one go
  for (TRowIterator it = Table.BegRI(); it < Table.EndRI(); it++) {
    if (!Collisions.IsKey(it.GetRowIdx())) {
      result->AddRow(it);
    }
  }

  // printf("this: %d %d, table: %d %d, result: %d %d\n",
  //   this->GetNumRows().Val, this->GetNumValidRows().Val,
  //   Table.GetNumRows().Val, Table.GetNumValidRows().Val,
  //   result->GetNumRows().Val, result->GetNumValidRows().Val);

  result->InitIds();
  return result;
}


PTable TTable::Intersection(const TTable& Table) {
  Schema NewSchema;
  THashSet<TInt64, int64> Collisions;

  for (TInt64 c = 0; c < Sch.Len(); c++) {
    if (Sch[c].Val1 != GetIdColName()) {
      NewSchema.Add(TPair<TStr,TAttrType>(Sch[c].Val1, Sch[c].Val2));
    }
  }
  PTable result = TTable::New(NewSchema, Context);

  GetCollidingRows(Table, Collisions);

  // this part should be made faster by adding all the rows in one go
  for (TRowIterator it = Table.BegRI(); it < Table.EndRI(); it++) {
    if (Collisions.IsKey(it.GetRowIdx())) {
      result->AddRow(it);
    }
  }
  result->InitIds();
  return result;
}

// TTable cannot be const because we will eventually call Table->GroupAux
// as of now, GroupAux cannot be const because it modifies the table in some cases
PTable TTable::Minus(TTable& Table) {
  Schema NewSchema;
  THashSet<TInt64, int64> Collisions;

  for (TInt64 c = 0; c < Sch.Len(); c++) {
    if (Sch[c].Val1 != GetIdColName()) {
      NewSchema.Add(TPair<TStr,TAttrType>(Sch[c].Val1, Sch[c].Val2));
    }
  }
  PTable result = TTable::New(NewSchema, Context);

  Table.GetCollidingRows(*this, Collisions);

  // this part should be made faster by adding all the rows in one go
  for (TRowIterator it = BegRI(); it < EndRI(); it++) {
    if (!Collisions.IsKey(it.GetRowIdx())) {
      result->AddRow(it);
    }
  }
  result->InitIds();
  return result;
}

PTable TTable::Project(const TStr64V& ProjectCols) {
  Schema NewSchema;
  for (TInt64 c = 0; c < ProjectCols.Len(); c++) {
    if (!IsColName(ProjectCols[c])) { TExcept::Throw("no such column " + ProjectCols[c]); }
    NewSchema.Add(TPair<TStr,TAttrType>(ProjectCols[c], GetColType(ProjectCols[c])));
  }

  PTable result = TTable::New(NewSchema, Context);
  result->AddTable(*this);
  result->InitIds();
  return result;
}

TBool TTable::IsAttr(const TStr& Attr) {
  return IsColName(Attr);
}

TStr TTable::RenumberColName(const TStr& ColName) const {
  TStr NColName = ColName;
  if (NColName.GetCh(NColName.Len()-2) == '-') {
    NColName = NColName.GetSubStr(0,NColName.Len()-3);
  }
  TInt64 Conflicts = 0;
  for (TInt64 i = 0; i < Sch.Len(); i++) {
    if (NColName == Sch[i].Val1.GetSubStr(0, Sch[i].Val1.Len()-3)) {
      Conflicts++;
    }
  }
  Conflicts++;
  NColName = NColName + "-" + Conflicts.GetStr();
  return NColName;
}

TStr TTable::DenormalizeColName(const TStr& ColName) const {
  TStr DColName = ColName;
  if (DColName.Len() == 0) { return DColName; }
  if (DColName.GetCh(0) == '_') { return DColName; }
  if (DColName.GetCh(DColName.Len()-2) == '-') {
    DColName = DColName.GetSubStr(0,DColName.Len()-3);
  }
  TInt64 Conflicts = 0;
  for (TInt64 i = 0; i < Sch.Len(); i++) {
    if (DColName == Sch[i].Val1.GetSubStr(0, Sch[i].Val1.Len()-3)) {
      Conflicts++;
    }
  }
  if (Conflicts > 1) { return ColName; }
  else { return DColName; }
}

Schema TTable::DenormalizeSchema() const {
  Schema DSch;
  for (TInt64 i = 0; i < Sch.Len(); i++) {
    DSch.Add(TPair<TStr, TAttrType>(DenormalizeColName(Sch[i].Val1), Sch[i].Val2));
  }
  return DSch;
}

void TTable::AddIntCol(const TStr& ColName) {
  AddSchemaCol(ColName, atInt);
  IntCols.Add(TInt64V(NumRows));
  TInt64 L = IntCols.Len();
  AddColType(ColName, atInt, L-1);
}

void TTable::AddFltCol(const TStr& ColName) {
  AddSchemaCol(ColName, atFlt);
  FltCols.Add(TFlt64V(NumRows));
  TInt64 L = FltCols.Len();
  AddColType(ColName, atFlt, L-1);
}

void TTable::AddStrCol(const TStr& ColName) {
  AddSchemaCol(ColName, atStr);
  StrColMaps.Add(TInt64V(NumRows));
  TInt64 L = StrColMaps.Len();
  AddColType(ColName, atStr, L-1);
}

void TTable::ClassifyAux(const TInt64V& SelectedRows, const TStr& LabelName, const TInt64& PositiveLabel, const TInt64& NegativeLabel) {
  AddSchemaCol(LabelName, atInt);
  TInt64 LabelColIdx = IntCols.Len();
  AddColType(LabelName, atInt, LabelColIdx);
  IntCols.Add(TInt64V(NumRows));
  for (TInt64 i = 0; i < NumRows; i++) {
    IntCols[LabelColIdx][i] = NegativeLabel;
  }
  for (TInt64 i = 0; i < SelectedRows.Len(); i++) {
    IntCols[LabelColIdx][SelectedRows[i]] = PositiveLabel;
  }
}

#ifdef USE_OPENMP
void TTable::ColGenericOpMP(TInt64 ArgColIdx1, TInt64 ArgColIdx2, TAttrType ArgType1, TAttrType ArgType2, TInt64 ResColIdx, TArithOp op){
  TAttrType ResType = atFlt;
  if(ArgType1 == atInt && ArgType2 == atInt){ ResType = atInt;}
  TIntPr64V Partitions;
  GetPartitionRanges(Partitions, omp_get_max_threads()*CHUNKS_PER_THREAD);
  //TInt64 PartitionSize = Partitions[0].GetVal2()-Partitions[0].GetVal1()+1;
  #pragma omp parallel for schedule(dynamic, CHUNKS_PER_THREAD)
  for (int64 i = 0; i < Partitions.Len(); i++){
    TRowIterator RowI(Partitions[i].GetVal1(), this);
    TRowIterator EndI(Partitions[i].GetVal2(), this);
    while(RowI < EndI){
      if(ResType == atInt){
        TInt64 V1 = RowI.GetIntAttr(ArgColIdx1);
        TInt64 V2 = RowI.GetIntAttr(ArgColIdx2);
        if (op == aoAdd) { IntCols[ResColIdx][RowI.GetRowIdx()] = V1 + V2; }
            if (op == aoSub) { IntCols[ResColIdx][RowI.GetRowIdx()] = V1 - V2; }
            if (op == aoMul) { IntCols[ResColIdx][RowI.GetRowIdx()] = V1 * V2; }
            if (op == aoDiv) { IntCols[ResColIdx][RowI.GetRowIdx()] = V1 / V2; }
            if (op == aoMod) { IntCols[ResColIdx][RowI.GetRowIdx()] = V1 % V2; }
            if (op == aoMin) { IntCols[ResColIdx][RowI.GetRowIdx()] = (V1 < V2) ? V1 : V2;}
            if (op == aoMax) { IntCols[ResColIdx][RowI.GetRowIdx()] = (V1 > V2) ? V1 : V2;}
      } else{
          TFlt V1 = (ArgType1 == atInt) ? (TFlt)RowI.GetIntAttr(ArgColIdx1) : RowI.GetFltAttr(ArgColIdx1);
          TFlt V2 = (ArgType2 == atInt) ? (TFlt)RowI.GetIntAttr(ArgColIdx2) : RowI.GetFltAttr(ArgColIdx2);
        if (op == aoAdd) { FltCols[ResColIdx][RowI.GetRowIdx()] = V1 + V2; }
            if (op == aoSub) { FltCols[ResColIdx][RowI.GetRowIdx()] = V1 - V2; }
            if (op == aoMul) { FltCols[ResColIdx][RowI.GetRowIdx()] = V1 * V2; }
            if (op == aoDiv) { FltCols[ResColIdx][RowI.GetRowIdx()] = V1 / V2; }
            if (op == aoMod) { TExcept::Throw("Cannot find modulo for float columns");  }
            if (op == aoMin) { FltCols[ResColIdx][RowI.GetRowIdx()] = (V1 < V2) ? V1 : V2;}
            if (op == aoMax) { FltCols[ResColIdx][RowI.GetRowIdx()] = (V1 > V2) ? V1 : V2;}
      }
      RowI++;
    }
  }
}
#endif  // USE_OPENMP

/* Performs generic operations on two numeric attributes
 * Operation can be +, -, *, /, %, min or max
 * Alternative is to write separate functions for each operation
 * Branch prediction may result in as fast performance anyway ?
 *
 */
void TTable::ColGenericOp(const TStr& Attr1, const TStr& Attr2, const TStr& ResAttr, TArithOp op) {
  // check if attributes are valid
  if (!IsAttr(Attr1)) TExcept::Throw("No attribute present: " + Attr1);
  if (!IsAttr(Attr2)) TExcept::Throw("No attribute present: " + Attr2);
  TPair<TAttrType, TInt64> Info1 = GetColTypeMap(Attr1);
  TPair<TAttrType, TInt64> Info2 = GetColTypeMap(Attr2);
  TAttrType Arg1Type = Info1.Val1;
  TAttrType Arg2Type = Info2.Val1;
  if (Arg1Type == atStr || Arg2Type == atStr) {
    TExcept::Throw("Only numeric columns supported in arithmetic operations.");
  }
  if(Arg1Type == atInt && Arg2Type == atFlt && ResAttr == ""){
    TExcept::Throw("Trying to write float values to an existing int-typed column");
  }
  // source column indices
  TInt64 ColIdx1 = Info1.Val2;
  TInt64 ColIdx2 = Info2.Val2;

  // destination column index
  TInt64 ColIdx3 = ColIdx1;
  // Create empty result column with type that of first attribute
  if (ResAttr != "") {
      if (Arg1Type == atInt && Arg2Type == atInt) {
          AddIntCol(ResAttr);
      }
      else {
          AddFltCol(ResAttr);
      }
      ColIdx3 = GetColIdx(ResAttr);
  }
#ifdef USE_OPENMP
  if(GetMP()){
    ColGenericOpMP(ColIdx1, ColIdx2, Arg1Type, Arg2Type, ColIdx3, op);
    return;
  }
#endif  //USE_OPENMP
  TAttrType ResType = atFlt;
  if(Arg1Type == atInt && Arg2Type == atInt){ printf("hooray!\n"); ResType = atInt;}
  for (TRowIterator RowI = BegRI(); RowI < EndRI(); RowI++) {
    //printf("%d %d %d %d\n", ColIdx1.Val, ColIdx2.Val, ColIdx3.Val, RowI.GetRowIdx().Val);
    if(ResType == atInt){
      TInt64 V1 = RowI.GetIntAttr(ColIdx1);
      TInt64 V2 = RowI.GetIntAttr(ColIdx2);
      if (op == aoAdd) { IntCols[ColIdx3][RowI.GetRowIdx()] = V1 + V2; }
          if (op == aoSub) { IntCols[ColIdx3][RowI.GetRowIdx()] = V1 - V2; }
          if (op == aoMul) { IntCols[ColIdx3][RowI.GetRowIdx()] = V1 * V2; }
          if (op == aoDiv) { IntCols[ColIdx3][RowI.GetRowIdx()] = V1 / V2; }
          if (op == aoMod) { IntCols[ColIdx3][RowI.GetRowIdx()] = V1 % V2; }
          if (op == aoMin) { IntCols[ColIdx3][RowI.GetRowIdx()] = (V1 < V2) ? V1 : V2;}
          if (op == aoMax) { IntCols[ColIdx3][RowI.GetRowIdx()] = (V1 > V2) ? V1 : V2;}
    } else{
      TFlt V1 = (Arg1Type == atInt) ? (TFlt)RowI.GetIntAttr(ColIdx1) : RowI.GetFltAttr(ColIdx1);
      TFlt V2 = (Arg2Type == atInt) ? (TFlt)RowI.GetIntAttr(ColIdx2) : RowI.GetFltAttr(ColIdx2);
      if (op == aoAdd) { FltCols[ColIdx3][RowI.GetRowIdx()] = V1 + V2; }
          if (op == aoSub) { FltCols[ColIdx3][RowI.GetRowIdx()] = V1 - V2; }
          if (op == aoMul) { FltCols[ColIdx3][RowI.GetRowIdx()] = V1 * V2; }
          if (op == aoDiv) { FltCols[ColIdx3][RowI.GetRowIdx()] = V1 / V2; }
          if (op == aoMod) { TExcept::Throw("Cannot find modulo for float columns");  }
          if (op == aoMin) { FltCols[ColIdx3][RowI.GetRowIdx()] = (V1 < V2) ? V1 : V2;}
          if (op == aoMax) { FltCols[ColIdx3][RowI.GetRowIdx()] = (V1 > V2) ? V1 : V2;}
    }
  }
}

void TTable::ColAdd(const TStr& Attr1, const TStr& Attr2, const TStr& ResultAttrName) {
  ColGenericOp(Attr1, Attr2, ResultAttrName, aoAdd);
}

void TTable::ColSub(const TStr& Attr1, const TStr& Attr2, const TStr& ResultAttrName) {
  ColGenericOp(Attr1, Attr2, ResultAttrName, aoSub);
}

void TTable::ColMul(const TStr& Attr1, const TStr& Attr2, const TStr& ResultAttrName) {
  ColGenericOp(Attr1, Attr2, ResultAttrName, aoMul);
}

void TTable::ColDiv(const TStr& Attr1, const TStr& Attr2, const TStr& ResultAttrName) {
  ColGenericOp(Attr1, Attr2, ResultAttrName, aoDiv);
}

void TTable::ColMod(const TStr& Attr1, const TStr& Attr2, const TStr& ResultAttrName) {
  ColGenericOp(Attr1, Attr2, ResultAttrName, aoMod);
}

void TTable::ColMin(const TStr& Attr1, const TStr& Attr2, const TStr& ResultAttrName) {
  ColGenericOp(Attr1, Attr2, ResultAttrName, aoMin);
}

void TTable::ColMax(const TStr& Attr1, const TStr& Attr2, const TStr& ResultAttrName) {
  ColGenericOp(Attr1, Attr2, ResultAttrName, aoMax);
}

void TTable::ColGenericOp(const TStr& Attr1, TTable& Table, const TStr& Attr2, const TStr& ResAttr,
 TArithOp op, TBool AddToFirstTable) {
  // check if attributes are valid
  if (!IsAttr(Attr1)) { TExcept::Throw("No attribute present: " + Attr1); }
  if (!Table.IsAttr(Attr2)) { TExcept::Throw("No attribute present: " + Attr2); }

  if (NumValidRows != Table.NumValidRows) {
    TExcept::Throw("Tables do not have equal number of rows");
  }

  TPair<TAttrType, TInt64> Info1 = GetColTypeMap(Attr1);
  TPair<TAttrType, TInt64> Info2 = Table.GetColTypeMap(Attr2);
  TAttrType Arg1Type = Info1.Val1;
  TAttrType Arg2Type = Info2.Val1;
  if (Info1.Val1 == atStr || Info2.Val1 == atStr) {
    TExcept::Throw("Only numeric columns supported in arithmetic operations.");
  }
  if(Arg1Type == atInt && Arg2Type == atFlt && ResAttr == ""){
    TExcept::Throw("Trying to write float values to an existing int-typed column");
  }
  // source column indices
  TInt64 ColIdx1 = Info1.Val2;
  TInt64 ColIdx2 = Info2.Val2;

  // destination column index
  TInt64 ColIdx3 = AddToFirstTable ? ColIdx1 : ColIdx2;

  // Create empty result column in appropriate table with type that of first attribute
  if (ResAttr != "") {
    if (AddToFirstTable) {
      if (Arg1Type == atInt && Arg2Type == atInt) {
          AddIntCol(ResAttr);
      } else {
          AddFltCol(ResAttr);
      }
      ColIdx3 = GetColIdx(ResAttr);
    }
    else {
      if (Arg1Type == atInt && Arg2Type == atInt) {
          Table.AddIntCol(ResAttr);
      } else {
          Table.AddFltCol(ResAttr);
      }
      ColIdx3 = Table.GetColIdx(ResAttr);
    }
  }

  /*
  #ifdef USE_OPENMP
  if(GetMP()){
    ColGenericOpMP(Table, AddToFirstTable, ColIdx1, ColIdx2, Arg1Type, Arg2Type, ColIdx3, op);
    return;
  }
  #endif  //USE_OPENMP
  */

  TRowIterator RI1, RI2;
  RI1 = BegRI();
  RI2 = Table.BegRI();
  TAttrType ResType = atFlt;
  if(Arg1Type == atInt && Arg2Type == atInt){ ResType = atInt;}
  while (RI1 < EndRI() && RI2 < Table.EndRI()) {
    if (ResType == atInt) {
    TInt64 V1 = RI1.GetIntAttr(ColIdx1);
    TInt64 V2 = RI2.GetIntAttr(ColIdx2);
        if (AddToFirstTable) {
          if (op == aoAdd) { IntCols[ColIdx3][RI1.GetRowIdx()] = V1 + V2; }
          if (op == aoSub) { IntCols[ColIdx3][RI1.GetRowIdx()] = V1 - V2; }
          if (op == aoMul) { IntCols[ColIdx3][RI1.GetRowIdx()] = V1 * V2; }
          if (op == aoDiv) { IntCols[ColIdx3][RI1.GetRowIdx()] = V1 / V2; }
           if (op == aoMod) { IntCols[ColIdx3][RI1.GetRowIdx()] = V1 % V2; }
        }
        else {
          if (op == aoAdd) { Table.IntCols[ColIdx3][RI2.GetRowIdx()] = V1 + V2; }
          if (op == aoSub) { Table.IntCols[ColIdx3][RI2.GetRowIdx()] = V1 - V2; }
          if (op == aoMul) { Table.IntCols[ColIdx3][RI2.GetRowIdx()] = V1 * V2; }
          if (op == aoDiv) { Table.IntCols[ColIdx3][RI2.GetRowIdx()] = V1 / V2; }
          if (op == aoMod) { Table.IntCols[ColIdx3][RI2.GetRowIdx()] = V1 % V2; }
        }
    } else {
      TFlt V1 = (Arg1Type == atInt) ? (TFlt)RI1.GetIntAttr(ColIdx1) : RI2.GetFltAttr(ColIdx1);
    TFlt V2 = (Arg2Type == atInt) ? (TFlt)RI1.GetIntAttr(ColIdx2) : RI2.GetFltAttr(ColIdx2);
        if (AddToFirstTable) {
          if (op == aoAdd) { FltCols[ColIdx3][RI1.GetRowIdx()] = V1 + V2; }
          if (op == aoSub) { FltCols[ColIdx3][RI1.GetRowIdx()] = V1 - V2; }
          if (op == aoMul) { FltCols[ColIdx3][RI1.GetRowIdx()] = V1 * V2; }
            if (op == aoDiv) { FltCols[ColIdx3][RI1.GetRowIdx()] = V1 / V2; }
          if (op == aoMod) { TExcept::Throw("Cannot find modulo for float columns"); }
        } else {
          if (op == aoAdd) { Table.FltCols[ColIdx3][RI2.GetRowIdx()] = V1 + V2; }
          if (op == aoSub) { Table.FltCols[ColIdx3][RI2.GetRowIdx()] = V1 - V2; }
          if (op == aoMul) { Table.FltCols[ColIdx3][RI2.GetRowIdx()] = V1 * V2; }
          if (op == aoDiv) { Table.FltCols[ColIdx3][RI2.GetRowIdx()] = V1 / V2; }
          if (op == aoMod) { TExcept::Throw("Cannot find modulo for float columns"); }
        }
    }
    RI1++;
    RI2++;
  }

  if (RI1 != EndRI() || RI2 != Table.EndRI()) {
    TExcept::Throw("ColGenericOp: Iteration error");
  }
}

void TTable::ColAdd(const TStr& Attr1, TTable& Table, const TStr& Attr2,
 const TStr& ResultAttrName, TBool AddToFirstTable) {
  ColGenericOp(Attr1, Table, Attr2, ResultAttrName, aoAdd, AddToFirstTable);
}

void TTable::ColSub(const TStr& Attr1, TTable& Table, const TStr& Attr2,
 const TStr& ResultAttrName, TBool AddToFirstTable) {
  ColGenericOp(Attr1, Table, Attr2, ResultAttrName, aoSub, AddToFirstTable);
}

void TTable::ColMul(const TStr& Attr1, TTable& Table, const TStr& Attr2,
 const TStr& ResultAttrName, TBool AddToFirstTable) {
  ColGenericOp(Attr1, Table, Attr2, ResultAttrName, aoMul, AddToFirstTable);
}

void TTable::ColDiv(const TStr& Attr1, TTable& Table, const TStr& Attr2,
 const TStr& ResultAttrName, TBool AddToFirstTable) {
  ColGenericOp(Attr1, Table, Attr2, ResultAttrName, aoDiv, AddToFirstTable);
}

void TTable::ColMod(const TStr& Attr1, TTable& Table, const TStr& Attr2,
 const TStr& ResultAttrName, TBool AddToFirstTable) {
  ColGenericOp(Attr1, Table, Attr2, ResultAttrName, aoMod, AddToFirstTable);
}


void TTable::ColGenericOp(const TStr& Attr1, const TFlt& Num, const TStr& ResAttr, TArithOp op, const TBool floatCast) {
  // check if attribute is valid
  if (!IsAttr(Attr1)) { TExcept::Throw("No attribute present: " + Attr1); }

  TPair<TAttrType, TInt64> Info1 = GetColTypeMap(Attr1);
  TAttrType ArgType = Info1.Val1;
  if (ArgType == atStr) {
    TExcept::Throw("Only numeric columns supported in arithmetic operations.");
  }
  // source column index
  TInt64 ColIdx1 = Info1.Val2;
  // destination column index
  TInt64 ColIdx2 = ColIdx1;

  // Create empty result column with type that of first attribute
  TBool shouldCast = floatCast;
  if (ResAttr != "") {
      if ((ArgType == atInt) & !shouldCast) {
          AddIntCol(ResAttr);
      } else {
          AddFltCol(ResAttr);
      }
      ColIdx2 = GetColIdx(ResAttr);
  } else {
    // Cannot change type of existing attribute
    shouldCast = false;
  }

  #ifdef USE_OPENMP
  if(GetMP()){
    ColGenericOpMP(ColIdx1, ColIdx2, ArgType, Num, op, shouldCast);
    return;
  }
  #endif  //USE_OPENMP

  for (TRowIterator RowI = BegRI(); RowI < EndRI(); RowI++) {
    if ((ArgType == atInt) && !shouldCast) {
      TInt64 CurVal = RowI.GetIntAttr(ColIdx1);
      TInt64 Val = static_cast<int64>(Num);
      if (op == aoAdd) { IntCols[ColIdx2][RowI.GetRowIdx()] = CurVal + Val; }
      if (op == aoSub) { IntCols[ColIdx2][RowI.GetRowIdx()] = CurVal - Val; }
      if (op == aoMul) { IntCols[ColIdx2][RowI.GetRowIdx()] = CurVal * Val; }
      if (op == aoDiv) { IntCols[ColIdx2][RowI.GetRowIdx()] = CurVal / Val; }
      if (op == aoMod) { IntCols[ColIdx2][RowI.GetRowIdx()] = CurVal % Val; }
    }
    else {
      TFlt CurVal = (ArgType == atFlt) ? RowI.GetFltAttr(ColIdx1) : (TFlt) RowI.GetIntAttr(ColIdx1);
      if (op == aoAdd) { FltCols[ColIdx2][RowI.GetRowIdx()] = CurVal + Num; }
      if (op == aoSub) { FltCols[ColIdx2][RowI.GetRowIdx()] = CurVal - Num; }
      if (op == aoMul) { FltCols[ColIdx2][RowI.GetRowIdx()] = CurVal * Num; }
      if (op == aoDiv) { FltCols[ColIdx2][RowI.GetRowIdx()] = CurVal / Num; }
      if (op == aoMod) { TExcept::Throw("Cannot find modulo for float columns"); }
    }
  }
}

#ifdef USE_OPENMP
void TTable::ColGenericOpMP(const TInt64& ColIdx1, const TInt64& ColIdx2, TAttrType ArgType, const TFlt& Num, TArithOp op, TBool ShouldCast){
  TIntPr64V Partitions;
  GetPartitionRanges(Partitions, omp_get_max_threads()*CHUNKS_PER_THREAD);
  //TInt64 PartitionSize = Partitions[0].GetVal2()-Partitions[0].GetVal1()+1;
  #pragma omp parallel for schedule(dynamic, CHUNKS_PER_THREAD)
  for (int64 i = 0; i < Partitions.Len(); i++){
    TRowIterator RowI(Partitions[i].GetVal1(), this);
    TRowIterator EndI(Partitions[i].GetVal2(), this);
    while(RowI < EndI){
      if ((ArgType == atInt) && !ShouldCast) {
            TInt64 CurVal = RowI.GetIntAttr(ColIdx1);
            TInt64 Val = static_cast<int64>(Num);
            if (op == aoAdd) { IntCols[ColIdx2][RowI.GetRowIdx()] = CurVal + Val; }
            if (op == aoSub) { IntCols[ColIdx2][RowI.GetRowIdx()] = CurVal - Val; }
            if (op == aoMul) { IntCols[ColIdx2][RowI.GetRowIdx()] = CurVal * Val; }
            if (op == aoDiv) { IntCols[ColIdx2][RowI.GetRowIdx()] = CurVal / Val; }
            if (op == aoMod) { IntCols[ColIdx2][RowI.GetRowIdx()] = CurVal % Val; }
        } else {
           TFlt CurVal = (ArgType == atFlt) ? RowI.GetFltAttr(ColIdx1) : (TFlt) RowI.GetIntAttr(ColIdx1);
            if (op == aoAdd) { FltCols[ColIdx2][RowI.GetRowIdx()] = CurVal + Num; }
            if (op == aoSub) { FltCols[ColIdx2][RowI.GetRowIdx()] = CurVal - Num; }
            if (op == aoMul) { FltCols[ColIdx2][RowI.GetRowIdx()] = CurVal * Num; }
            if (op == aoDiv) { FltCols[ColIdx2][RowI.GetRowIdx()] = CurVal / Num; }
            if (op == aoMod) { TExcept::Throw("Cannot find modulo for float columns"); }
        }
        RowI++;
    }
  }
}
#endif

void TTable::ColAdd(const TStr& Attr1, const TFlt& Num, const TStr& ResultAttrName, const TBool floatCast) {
  ColGenericOp(Attr1, Num, ResultAttrName, aoAdd, floatCast);
}

void TTable::ColSub(const TStr& Attr1, const TFlt& Num, const TStr& ResultAttrName, const TBool floatCast) {
  ColGenericOp(Attr1, Num, ResultAttrName, aoSub, floatCast);
}

void TTable::ColMul(const TStr& Attr1, const TFlt& Num, const TStr& ResultAttrName, const TBool floatCast) {
  ColGenericOp(Attr1, Num, ResultAttrName, aoMul, floatCast);
}

void TTable::ColDiv(const TStr& Attr1, const TFlt& Num, const TStr& ResultAttrName, const TBool floatCast) {
  ColGenericOp(Attr1, Num, ResultAttrName, aoDiv, floatCast);
}

void TTable::ColMod(const TStr& Attr1, const TFlt& Num, const TStr& ResultAttrName, const TBool floatCast) {
  ColGenericOp(Attr1, Num, ResultAttrName, aoMod, floatCast);
}

void TTable::ColConcat(const TStr& Attr1, const TStr& Attr2, const TStr& Sep, const TStr& ResAttr) {
  // check if attributes are valid
  if (!IsAttr(Attr1)) TExcept::Throw("No attribute present: " + Attr1);
  if (!IsAttr(Attr2)) TExcept::Throw("No attribute present: " + Attr2);

  TPair<TAttrType, TInt64> Info1 = GetColTypeMap(Attr1);
  TPair<TAttrType, TInt64> Info2 = GetColTypeMap(Attr2);

  if (Info1.Val1 != atStr || Info2.Val1 != atStr) {
    TExcept::Throw("Only string columns supported in concat.");
  }

  // source column indices
  TInt64 ColIdx1 = Info1.Val2;
  TInt64 ColIdx2 = Info2.Val2;

  // destination column index
  TInt64 ColIdx3 = ColIdx1;

  // Create empty result column with type that of first attribute
  if (ResAttr != "") {
      AddStrCol(ResAttr);
      ColIdx3 = GetColIdx(ResAttr);
  }

  for (TRowIterator RowI = BegRI(); RowI < EndRI(); RowI++) {
    TStr CurVal1 = RowI.GetStrAttr(ColIdx1);
    TStr CurVal2 = RowI.GetStrAttr(ColIdx2);
    TStr NewVal = CurVal1 + Sep + CurVal2;
    TInt64 Key = TInt64(Context->StringVals.AddKey(NewVal));
    StrColMaps[ColIdx3][RowI.GetRowIdx()] = Key;
  }
}

void TTable::ColConcat(const TStr& Attr1, TTable& Table, const TStr& Attr2, const TStr& Sep,
 const TStr& ResAttr, TBool AddToFirstTable) {
  // check if attributes are valid
  if (!IsAttr(Attr1)) { TExcept::Throw("No attribute present: " + Attr1); }
  if (!Table.IsAttr(Attr2)) { TExcept::Throw("No attribute present: " + Attr2); }

  if (NumValidRows != Table.NumValidRows) {
    TExcept::Throw("Tables do not have equal number of rows");
  }

  TPair<TAttrType, TInt64> Info1 = GetColTypeMap(Attr1);
  TPair<TAttrType, TInt64> Info2 = Table.GetColTypeMap(Attr2);

  if (Info1.Val1 != atStr || Info2.Val1 != atStr) {
    TExcept::Throw("Only string columns supported in concat.");
  }

  // source column indices
  TInt64 ColIdx1 = Info1.Val2;
  TInt64 ColIdx2 = Info2.Val2;

  // destination column index
  TInt64 ColIdx3 = ColIdx1;

  if (!AddToFirstTable) {
    ColIdx3 = ColIdx2;
  }

  // Create empty result column in appropriate table with type that of first attribute
  if (ResAttr != "") {
    if (AddToFirstTable) {
      AddStrCol(ResAttr);
      ColIdx3 = GetColIdx(ResAttr);
    }
    else {
      Table.AddStrCol(ResAttr);
      ColIdx3 = Table.GetColIdx(ResAttr);
    }
  }

  TRowIterator RI1, RI2;

  RI1 = BegRI();
  RI2 = Table.BegRI();

  while (RI1 < EndRI() && RI2 < Table.EndRI()) {
    TStr CurVal1 = RI1.GetStrAttr(ColIdx1);
    TStr CurVal2 = RI2.GetStrAttr(ColIdx2);
    TStr NewVal = CurVal1 + Sep + CurVal2;
    TInt64 Key = TInt64(Context->StringVals.AddKey(NewVal));
    if (AddToFirstTable) {
      StrColMaps[ColIdx3][RI1.GetRowIdx()] = Key;
    }
    else {
      Table.StrColMaps[ColIdx3][RI2.GetRowIdx()] = Key;
    }
    RI1++;
    RI2++;
  }

  if (RI1 != EndRI() || RI2 != Table.EndRI()) {
    TExcept::Throw("ColGenericOp: Iteration error");
  }
}

void TTable::ColConcatConst(const TStr& Attr1, const TStr& Val, const TStr& Sep, const TStr& ResAttr) {
  // check if attribute is valid
  if (!IsAttr(Attr1)) { TExcept::Throw("No attribute present: " + Attr1); }

  TPair<TAttrType, TInt64> Info1 = GetColTypeMap(Attr1);

  if (Info1.Val1 != atStr) {
    TExcept::Throw("Only string columns supported in concat.");
  }

  // source column index
  TInt64 ColIdx1 = Info1.Val2;

  // destination column index
  TInt64 ColIdx2 = ColIdx1;

  // Create empty result column with type that of first attribute
  if (ResAttr != "") {
    AddStrCol(ResAttr);
    ColIdx2 = GetColIdx(ResAttr);
  }

  for (TRowIterator RowI = BegRI(); RowI < EndRI(); RowI++) {
    TStr CurVal = RowI.GetStrAttr(ColIdx1);
    TStr NewVal = CurVal + Sep + Val;
    TInt64 Key = TInt64(Context->StringVals.AddKey(NewVal));
    StrColMaps[ColIdx2][RowI.GetRowIdx()] = Key;
  }
}

void TTable::ReadIntCol(const TStr& ColName, TInt64V& Result) const{
  if (!IsColName(ColName)) { TExcept::Throw("no such column " + ColName); }
  if (GetColType(ColName) != atInt) { TExcept::Throw("not an integer column " + ColName); }
  TInt64 ColId = GetColIdx(ColName);
  for (TRowIterator it = BegRI(); it < EndRI(); it++) {
    Result.Add(it.GetIntAttr(ColId));
  }
}

void TTable::ReadFltCol(const TStr& ColName, TFlt64V& Result) const{
  if (!IsColName(ColName)) { TExcept::Throw("no such column " + ColName); }
  if (GetColType(ColName) != atFlt) { TExcept::Throw("not a floating point column " + ColName); }
  TInt64 ColId = GetColIdx(ColName);
  for (TRowIterator it = BegRI(); it < EndRI(); it++) {
    Result.Add(it.GetFltAttr(ColId));
  }
}

void TTable::ReadStrCol(const TStr& ColName, TStr64V& Result) const{
  if (!IsColName(ColName)) { TExcept::Throw("no such column " + ColName); }
  if (GetColType(ColName) != atStr) { TExcept::Throw("not a string column " + ColName); }
  TInt64 ColId = GetColIdx(ColName);
  for (TRowIterator it = BegRI(); it < EndRI(); it++) {
    Result.Add(it.GetStrAttr(ColId));
  }
}

void TTable::ProjectInPlace(const TStr64V& ProjectCols) {
  TStr64V NProjectCols = NormalizeColNameV(ProjectCols);
  for (TInt64 c = 0; c < NProjectCols.Len(); c++) {
    if (!IsColName(NProjectCols[c])) { TExcept::Throw("no such column " + NProjectCols[c]); }
  }
  THashSet<TStr, int64> ProjectColsSet = THashSet<TStr, int64>(NProjectCols);
  // Delete the column vectors
  for (TInt64 i = Sch.Len() - 1; i >= 0; i--) {
    TStr ColName = GetSchemaColName(i);
    if (ProjectColsSet.IsKey(ColName) || ColName == IdColName) { continue; }
    TAttrType ColType = GetSchemaColType(i);
    TInt64 ColId = GetColIdx(ColName);
    switch (ColType) {
      case atInt:
        IntCols.Del(ColId);
        break;
      case atFlt:
        FltCols.Del(ColId);
        break;
      case atStr:
        StrColMaps.Del(ColId);
        break;
    }
  }

  // Rebuild the ColTypeMap with new indexes of the column vectors
  TInt64 IntColCnt = 0;
  TInt64 FltColCnt = 0;
  TInt64 StrColCnt = 0;
  ColTypeMap.Clr();
  for (TInt64 i = 0; i < Sch.Len(); i++) {
    TStr ColName = GetSchemaColName(i);
    if (!ProjectColsSet.IsKey(ColName) && ColName != IdColName) { continue; }
    TAttrType ColType = GetSchemaColType(i);
    switch (ColType) {
      case atInt:
        AddColType(ColName, atInt, IntColCnt);
        IntColCnt++;
        break;
      case atFlt:
        AddColType(ColName, atFlt, FltColCnt);
        FltColCnt++;
        break;
      case atStr:
        AddColType(ColName, atStr, StrColCnt);
        StrColCnt++;
        break;
    }
  }

  // Update schema
  for (TInt64 i = Sch.Len() - 1; i >= 0; i--) {
    TStr ColName = GetSchemaColName(i);
    if (ProjectColsSet.IsKey(ColName) || ColName == IdColName) { continue; }
    Sch.Del(i);
  }
}

TInt64 TTable::CompareKeyVal(const TInt64& K1, const TInt64& V1, const TInt64& K2, const TInt64& V2) {
  // if (K1 == K2) {
  //   if (V1 < V2) { return -1; }
  //   else if (V1 > V2) { return 1; }
  //   else return 0;
  // }
  // if (K1 < K2) { return -1; }
  // else { return 1; }

  if (K1 == K2) { return V1 - V2; }
  else { return K1 - K2; }
}

TInt64 TTable::CheckSortedKeyVal(TInt64V& Key, TInt64V& Val, TInt64 Start, TInt64 End) {
  TInt64 j;
  for (j = Start; j < End; j++) {
    if (CompareKeyVal(Key[j], Val[j], Key[j+1], Val[j+1]) > 0) {
      break;
    }
  }
  if (j >= End) { return 0; }
  else { return 1; }
}

void TTable::ISortKeyVal(TInt64V& Key, TInt64V& Val, TInt64 Start, TInt64 End) {
  if (Start < End) {
    for (TInt64 i = Start+1; i <= End; i++) {
      TInt64 K = Key[i];
      TInt64 V = Val[i];
      TInt64 j = i;
      while ((Start < j) && (CompareKeyVal(Key[j-1], Val[j-1], K, V) > 0)) {
        Key[j] = Key[j-1];
        Val[j] = Val[j-1];
        j--;
      }
      Key[j] = K;
      Val[j] = V;
    }
  }
}

TInt64 TTable::GetPivotKeyVal(TInt64V& Key, TInt64V& Val, TInt64 Start, TInt64 End) {
  TInt64 L = End - Start + 1;
  //CHECK GetRnd
  const TInt64 Idx1 = Start + TInt::GetRnd(L);
  const TInt64 Idx2 = Start + TInt::GetRnd(L);
  const TInt64 Idx3 = Start + TInt::GetRnd(L);
  if (CompareKeyVal(Key[Idx1], Val[Idx1], Key[Idx2], Val[Idx2]) < 0) {
    if (CompareKeyVal(Key[Idx2], Val[Idx2], Key[Idx3], Val[Idx3]) < 0) { return Idx2; }
    if (CompareKeyVal(Key[Idx1], Val[Idx1], Key[Idx3], Val[Idx3]) < 0) { return Idx3; }
    return Idx1;
  } else {
    if (CompareKeyVal(Key[Idx3], Val[Idx3], Key[Idx2], Val[Idx2]) < 0) { return Idx2; }
    if (CompareKeyVal(Key[Idx3], Val[Idx3], Key[Idx1], Val[Idx1]) < 0) { return Idx3; }
    return Idx1;
  }
}


TInt64 TTable::PartitionKeyVal(TInt64V& Key, TInt64V& Val, TInt64 Start, TInt64 End) {
  TInt64 Pivot = GetPivotKeyVal(Key, Val, Start, End);
  //printf("Pivot=%d\n", Pivot.Val);
  TInt64 PivotKey = Key[Pivot];
  TInt64 PivotVal = Val[Pivot];
  Key.Swap(Pivot, End);
  Val.Swap(Pivot, End);
  TInt64 StoreIdx = Start;
  for (TInt64 i = Start; i < End; i++) {
    //printf("%d %d %d %d\n", Key[i].Val, Val[i].Val, PivotKey.Val, PivotVal.Val);
    if (CompareKeyVal(Key[i], Val[i], PivotKey, PivotVal) <= 0) {
      Key.Swap(i, StoreIdx);
      Val.Swap(i, StoreIdx);
      StoreIdx++;
    }
  }
  //printf("StoreIdx=%d\n", StoreIdx.Val);
  // move pivot value to its place
  Key.Swap(StoreIdx, End);
  Val.Swap(StoreIdx, End);
  return StoreIdx;
}

void TTable::QSortKeyVal(TInt64V& Key, TInt64V& Val, TInt64 Start, TInt64 End) {
  //printf("Thread=%d, Start=%d, End=%d\n", omp_get_thread_num(), Start.Val, End.Val);
  TInt64 L = End-Start;
  if (L <= 0) { return; }
  if (CheckSortedKeyVal(Key, Val, Start, End) == 0) { return; }

  if (L <= 20) { ISortKeyVal(Key, Val, Start, End); }
  else {
    TInt64 Pivot = PartitionKeyVal(Key, Val, Start, End);

    if (Pivot > End) { return; }
    if (L <= 500000) {
      QSortKeyVal(Key, Val, Start, Pivot-1);
      QSortKeyVal(Key, Val, Pivot+1, End);
    } else {
#ifdef USE_OPENMP
#ifndef GLib_WIN32
      #pragma omp task untied shared(Key, Val)
#endif
#endif
      { QSortKeyVal(Key, Val, Start, Pivot-1); }

#ifdef USE_OPENMP
#ifndef GLib_WIN32
      #pragma omp task untied shared(Key, Val)
#endif
#endif
      { QSortKeyVal(Key, Val, Pivot+1, End); }
    }
  }
}

TInt64V TTable::GetIntRowIdxByVal(const TStr& ColName, const TInt64& Val) const {

  if (IntColIndexes.IsKey(ColName)) {
    THash<TInt64, TInt64V, int64> ColIndex = IntColIndexes.GetDat(ColName);
    if (ColIndex.IsKey(Val)) {
      return ColIndex.GetDat(Val);
    }
    else {
      TInt64V Empty;
      return Empty;
    }
  }
  TInt64V ToReturn;
  for (TRowIterator RowI = BegRI(); RowI < EndRI(); RowI++) {
    TInt64 ValAtRow = RowI.GetIntAttr(ColName);
    if ( Val == ValAtRow) {
      ToReturn.Add(RowI.GetRowIdx());
    }
  }
  return ToReturn;
}
TInt64V TTable::GetStrRowIdxByMap(const TStr& ColName, const TInt64& Map) const {

  if (StrMapColIndexes.IsKey(ColName)) {
    THash<TInt64, TInt64V, int64> ColIndex = StrMapColIndexes.GetDat(ColName);
    if (ColIndex.IsKey(Map)) {
      return ColIndex.GetDat(Map);
    }
    else {
      TInt64V Empty;
      return Empty;
    }
  }
  TInt64V ToReturn;
  for (TRowIterator RowI = BegRI(); RowI < EndRI(); RowI++) {
    TInt64 MapAtRow = RowI.GetStrMapByName(ColName);
    if ( Map == MapAtRow) {
      ToReturn.Add(RowI.GetRowIdx());
    }
  }
  return ToReturn;
}

TInt64V TTable::GetFltRowIdxByVal(const TStr& ColName, const TFlt& Val) const {

  if (FltColIndexes.IsKey(ColName)) {
    THash<TFlt, TInt64V, int64> ColIndex = FltColIndexes.GetDat(ColName);
    if (ColIndex.IsKey(Val)) {
      return ColIndex.GetDat(Val);
    }
    else {
      TInt64V Empty;
      return Empty;
    }
  }

  TInt64V ToReturn;
  for (TRowIterator RowI = BegRI(); RowI < EndRI(); RowI++) {
    TFlt ValAtRow = RowI.GetFltAttr(ColName);
    if ( Val == ValAtRow) {
      ToReturn.Add(RowI.GetRowIdx());
    }
  }
  return ToReturn;
}

TInt64 TTable::RequestIndexInt(const TStr& ColName) {

  THash<TInt64, TInt64V, int64> NewIndex;
  for (TRowIterator RowI = BegRI(); RowI < EndRI(); RowI++) {
    TInt64 ValAtRow = RowI.GetIntAttr(ColName);
    TInt64 RowIdx = RowI.GetRowIdx();
    if (NewIndex.IsKey(ValAtRow)) {
       TInt64V Curr_V = NewIndex.GetDat(ValAtRow);
       Curr_V.Add(RowIdx);
    }
    else {
      TInt64V New_V;
      New_V.Add(RowIdx);
      NewIndex.AddDat(ValAtRow, New_V);
    }
  }
  IntColIndexes.AddDat(ColName, NewIndex);
  return 0;
}
TInt64 TTable::RequestIndexFlt(const TStr& ColName) {

  THash<TFlt, TInt64V, int64> NewIndex;
  for (TRowIterator RowI = BegRI(); RowI < EndRI(); RowI++) {
    TFlt ValAtRow = RowI.GetFltAttr(ColName);
    TInt64 RowIdx = RowI.GetRowIdx();
    if (NewIndex.IsKey(ValAtRow)) {
       TInt64V Curr_V = NewIndex.GetDat(ValAtRow);
       Curr_V.Add(RowIdx);
    }
    else {
      TInt64V New_V;
      New_V.Add(RowIdx);
      NewIndex.AddDat(ValAtRow, New_V);
    }
  }
  FltColIndexes.AddDat(ColName, NewIndex);
  return 0;
}
TInt64 TTable::RequestIndexStrMap(const TStr& ColName) {
  THash<TInt64, TInt64V, int64> NewIndex;
  for (TRowIterator RowI = BegRI(); RowI < EndRI(); RowI++) {
    TInt64 MapAtRow = RowI.GetStrMapByName(ColName);
    TInt64 RowIdx = RowI.GetRowIdx();
    if (NewIndex.IsKey(MapAtRow)) {
       TInt64V Curr_V = NewIndex.GetDat(MapAtRow);
       Curr_V.Add(RowIdx);
    }
    else {
      TInt64V New_V;
      New_V.Add(RowIdx);
      NewIndex.AddDat(MapAtRow, New_V);
    }
  }
  StrMapColIndexes.AddDat(ColName, NewIndex);
  return 0;
}
