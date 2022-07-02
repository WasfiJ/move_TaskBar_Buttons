
// opt.hpp
// Copyright (c) 2022 Wasfi JAOUAD. All rights reserved.
// v0.3b 2022.06
// Getopt for humans : an explicit, easy to use C++ getopt (C++20)
// 
// It is so explicit, we only need to agree on :
//  optList(a,s)
//  user call: p.exe -a f.txt -s
//  -> argument#1 = "-a"     argument#2 = "f.txt"    argument#3 = "-s"
//     options : -a, -s   (arg1 and arg3)
//     "f.txt" is the argument of option "-a"
//
// Example use (erase or swap two lines of a text file):
// 
//      optList(f, n, e, s);  // -> optCnt == 4
//      optsNeedArgByDefault = true;   // all options must have an argument ! (disable per opt with optHasNoArg)
//      optAdd(              
//           ( f, ("-f", "--file", "-file"),    optMandatory )  
//          ,( n, ("-n", "--line-number"),      optMandatory & optCanRepeat )
//          ,( e, ("-e", "--erase", "-erase"),  optHasNoArg  )
//          ,( s, ("-s", "--swap", "-swap"),    optHasNoArg  )  // error if commented : all optListed options must be defined
//      );
//
//      optsMustHaveOneOf( e, s );
// 
//      optsRelation( optExcludeEachOther, (n, e, L"Error: swap or erase, not both") );  // custom error msg
//      optsRelation( optRequires,         (n, a), (e, a), (s, a));  // see enum class optRel (here it is useless, since -a is optMandatory)
//
//      optSetIndicator( (s, SWAP), (n, NBR) );  // your bool SWAP = true if "-s" used, for your own processing of args (keep yourself informed)
// 
//      OPT_GRACEFUL = true;  // be tolerant with errant unlawful arguments that do no harm
//      optLoad(argc, argv);   // all of your rules apply, rc == 0 -> all rules satisfied
//      bool notGraceful = false;  // if you want to override OPT_GRACEFUL
//      int rc = optNoExtraArgs(argv, notGraceful)  // severely check for errant args (extra args, unsupported options)
// 
// call : p.exe -file "" "_" "__" f.txt -n 2,17 -s extraArg -p    ("_" : called by a script that passes arg. "$var", and $var is empty)
//   ->  optArgi[f] == 5 (index in argv of -f's argument), optByUser[f] == "-file" (form chosen by the user)
//       Warning/error (depends on OPT_GRACEFUL) messages for "extraArg" (-s optHasNoArg) and "-p" (unsupported option)
// 
// TODO: optCanRepeat, trailing arguments

#include <vector>
#include <algorithm>


#define FE_0(WHAT)
#define FE_1(WHAT, X) WHAT(X) 
#define FE_2(WHAT, X, ...) WHAT(X)FE_1(WHAT, __VA_ARGS__)
#define FE_3(WHAT, X, ...) WHAT(X)FE_2(WHAT, __VA_ARGS__)
#define FE_4(WHAT, X, ...) WHAT(X)FE_3(WHAT, __VA_ARGS__)
#define FE_5(WHAT, X, ...) WHAT(X)FE_4(WHAT, __VA_ARGS__)
#define FE_6(WHAT, X, ...) WHAT(X)FE_5(WHAT, __VA_ARGS__)
#define FE_7(WHAT, X, ...) WHAT(X)FE_6(WHAT, __VA_ARGS__)
#define FE_8(WHAT, X, ...) WHAT(X)FE_7(WHAT, __VA_ARGS__)
#define FE_9(WHAT, X, ...) WHAT(X)FE_8(WHAT, __VA_ARGS__)
#define FE_10(WHAT, X, ...) WHAT(X)FE_9(WHAT, __VA_ARGS__)
#define FE_11(WHAT, X, ...) WHAT(X)FE_10(WHAT, __VA_ARGS__)
#define FE_12(WHAT, X, ...) WHAT(X)FE_11(WHAT, __VA_ARGS__)
#define FE_13(WHAT, X, ...) WHAT(X)FE_12(WHAT, __VA_ARGS__)
#define FE_14(WHAT, X, ...) WHAT(X)FE_13(WHAT, __VA_ARGS__)
#define FE_15(WHAT, X, ...) WHAT(X)FE_14(WHAT, __VA_ARGS__)
#define FE_16(WHAT, X, ...) WHAT(X)FE_15(WHAT, __VA_ARGS__)
#define FE_17(WHAT, X, ...) WHAT(X)FE_16(WHAT, __VA_ARGS__)
#define FE_18(WHAT, X, ...) WHAT(X)FE_17(WHAT, __VA_ARGS__)
#define FE_19(WHAT, X, ...) WHAT(X)FE_18(WHAT, __VA_ARGS__)
#define FE_20(WHAT, X, ...) WHAT(X)FE_19(WHAT, __VA_ARGS__)
#define FE_21(WHAT, X, ...) WHAT(X)FE_20(WHAT, __VA_ARGS__)
#define FE_22(WHAT, X, ...) WHAT(X)FE_21(WHAT, __VA_ARGS__)
#define FE_23(WHAT, X, ...) WHAT(X)FE_22(WHAT, __VA_ARGS__)
#define FE_24(WHAT, X, ...) WHAT(X)FE_23(WHAT, __VA_ARGS__)
#define FE_25(WHAT, X, ...) WHAT(X)FE_24(WHAT, __VA_ARGS__)
#define FE_26(WHAT, X, ...) WHAT(X)FE_25(WHAT, __VA_ARGS__)
#define FE_27(WHAT, X, ...) WHAT(X)FE_26(WHAT, __VA_ARGS__)
#define FE_28(WHAT, X, ...) WHAT(X)FE_27(WHAT, __VA_ARGS__)
#define FE_29(WHAT, X, ...) WHAT(X)FE_28(WHAT, __VA_ARGS__)
#define FE_30(WHAT, X, ...) WHAT(X)FE_29(WHAT, __VA_ARGS__)
#define FE_31(WHAT, X, ...) WHAT(X)FE_30(WHAT, __VA_ARGS__)
#define FE_32(WHAT, X, ...) WHAT(X)FE_31(WHAT, __VA_ARGS__)
#define FE_33(WHAT, X, ...) WHAT(X)FE_32(WHAT, __VA_ARGS__)
#define FE_34(WHAT, X, ...) WHAT(X)FE_33(WHAT, __VA_ARGS__)
#define FE_35(WHAT, X, ...) WHAT(X)FE_34(WHAT, __VA_ARGS__)
#define FE_36(WHAT, X, ...) WHAT(X)FE_35(WHAT, __VA_ARGS__)
#define FE_37(WHAT, X, ...) WHAT(X)FE_36(WHAT, __VA_ARGS__)
#define FE_38(WHAT, X, ...) WHAT(X)FE_37(WHAT, __VA_ARGS__)
#define FE_39(WHAT, X, ...) WHAT(X)FE_38(WHAT, __VA_ARGS__)
#define FE_40(WHAT, X, ...) WHAT(X)FE_39(WHAT, __VA_ARGS__)
#define FE_41(WHAT, X, ...) WHAT(X)FE_40(WHAT, __VA_ARGS__)
#define FE_42(WHAT, X, ...) WHAT(X)FE_41(WHAT, __VA_ARGS__)
#define FE_43(WHAT, X, ...) WHAT(X)FE_42(WHAT, __VA_ARGS__)
#define FE_44(WHAT, X, ...) WHAT(X)FE_43(WHAT, __VA_ARGS__)
#define FE_45(WHAT, X, ...) WHAT(X)FE_44(WHAT, __VA_ARGS__)
#define FE_46(WHAT, X, ...) WHAT(X)FE_45(WHAT, __VA_ARGS__)
#define FE_47(WHAT, X, ...) WHAT(X)FE_46(WHAT, __VA_ARGS__)
#define FE_48(WHAT, X, ...) WHAT(X)FE_47(WHAT, __VA_ARGS__)
#define FE_49(WHAT, X, ...) WHAT(X)FE_48(WHAT, __VA_ARGS__)
#define FE_50(WHAT, X, ...) WHAT(X)FE_49(WHAT, __VA_ARGS__)
#define FE_51(WHAT, X, ...) WHAT(X)FE_50(WHAT, __VA_ARGS__)
#define FE_52(WHAT, X, ...) WHAT(X)FE_51(WHAT, __VA_ARGS__)
#define FE_53(WHAT, X, ...) WHAT(X)FE_52(WHAT, __VA_ARGS__)
#define FE_54(WHAT, X, ...) WHAT(X)FE_53(WHAT, __VA_ARGS__)
#define FE_55(WHAT, X, ...) WHAT(X)FE_54(WHAT, __VA_ARGS__)
#define FE_56(WHAT, X, ...) WHAT(X)FE_55(WHAT, __VA_ARGS__)
#define FE_57(WHAT, X, ...) WHAT(X)FE_56(WHAT, __VA_ARGS__)
#define FE_58(WHAT, X, ...) WHAT(X)FE_57(WHAT, __VA_ARGS__)
#define FE_59(WHAT, X, ...) WHAT(X)FE_58(WHAT, __VA_ARGS__)
#define FE_60(WHAT, X, ...) WHAT(X)FE_59(WHAT, __VA_ARGS__)
#define FE_61(WHAT, X, ...) WHAT(X)FE_60(WHAT, __VA_ARGS__)
#define FE_62(WHAT, X, ...) WHAT(X)FE_61(WHAT, __VA_ARGS__)
#define FE_63(WHAT, X, ...) WHAT(X)FE_62(WHAT, __VA_ARGS__)

#define GET_MACRO(_0,_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26, \
  _27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,   \
  _55,_56,_57,_58,_59,_60,_61,_62,_63,NAME,...) NAME 
#define FOR_EACH(action,...) \
  GET_MACRO(_0,__VA_ARGS__,FE_63,FE_62,FE_61,FE_60,FE_59,FE_58,FE_57,FE_56,FE_55,FE_54,FE_53,FE_52,FE_51,FE_50,FE_49, \
  FE_48,FE_47,FE_46,FE_45,FE_44,FE_43,FE_42,FE_41,FE_40,FE_39,FE_38,FE_37,FE_36,FE_35,FE_34,FE_33,FE_32,FE_31,FE_30,  \
  FE_29,FE_28,FE_27,FE_26,FE_25,FE_24,FE_23,FE_22,FE_21,FE_20,FE_19,FE_18,FE_17,FE_16,FE_15,FE_14,FE_13,FE_12,FE_11,  \
  FE_10,FE_9,FE_8,FE_7,FE_6,FE_5,FE_4,FE_3,FE_2,FE_1,FE_0)(action,__VA_ARGS__)

#define PP_NARG(...) \
         PP_NARG_(__VA_ARGS__,PP_RSEQ_N())
#define PP_NARG_(...) \
         PP_ARG_N(__VA_ARGS__)
#define PP_ARG_N( \
          _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
         _61,_62,_63,N,...) N
#define PP_RSEQ_N() \
         63,62,61,60,                   \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1,0

#define vectFind(v,i) (std::find(begin(v), end(v), (i)))
#define vectContains(v,i) ((vectFind(v,i)) != std::end(v))
#define vectContainsStr(v,i) (v.end()!=std::find_if(v.begin(), v.end(), [i](const auto m)->bool{ return 0==strcmp(i, m); }))

short optCnt = 0, optCurrentArg = 0;
#define optNoSpec    0
#define optMandatory 0b0001
#define optCanRepeat 0b0010
#define optNeedsArg  0b0100
#define optHasNoArg  0b1000
struct Opt{ short id = 0; short argi = 0;
  vector<LPCSTR> *t = nullptr;
  int traits = optNoSpec;
  bool *bPresence = nullptr;
  vector<int> occurences; 
};
static vector<Opt *> Opts; static short* optArgi, *optidArgs;  vector<short> optArgNotAnOpt; static short *optRedefinitions;
static LPCSTR *optByUser; static vector<short> optByUserI; static vector<vector<short>> optVIPs;
static short *opt2Argi, *opt2idT, *optArgi2Opt; static bool optsNeedArgByDefault = false, OPT_GRACEFUL = false;
#pragma warning (disable: 4003)
#define UNPACK(...) __VA_ARGS__
#define _2_ARGS(id,text)   if(Opts[o_##id]->t) optRedefinitions[o_##id] = 1; (Opts[o_##id])->t = new vector<LPCSTR>({ UNPACK text }); \
  (Opts[o_##id])->traits |= (optsNeedArgByDefault? optNeedsArg : optNoSpec); 
#define _3_ARGS(id,text,v) if(Opts[o_##id]->t) optRedefinitions[o_##id] = 1; (Opts[o_##id])->t = new vector<LPCSTR>({ UNPACK text }); \
  if(v & optHasNoArg) (Opts[o_##id])->traits |= v; else (Opts[o_##id])->traits |= v | (optsNeedArgByDefault? optNeedsArg : optNoSpec);
#define GET_4TH_ARG(arg1, arg2, arg3, arg4, ...) arg4
#define _MACRO_CHOOSER(...) GET_4TH_ARG(__VA_ARGS__, _3_ARGS, _2_ARGS)
#define _MACRO_CHOOSER0(...) _MACRO_CHOOSER __VA_ARGS__ __VA_ARGS__
#define optAdd(...) FOR_EACH(_MACRO_CHOOSER0, __VA_ARGS__)
// optAdd(cg, ("-cg", "--change-group"), chgGroup);

#define optSetIndicator1(id,v) (*(Opts[o_##id])).bPresence = & v; 
#define optSetIndicator11(X) optSetIndicator1 X
#define optSetIndicator(...) FOR_EACH(optSetIndicator11, __VA_ARGS__)

#define optMakeComma1(X) X,
#define optMakeCommaList(X,...) FOR_EACH(optMakeComma1,__VA_ARGS__) X
#define optFill1(X)        Opts.push_back(new Opt({ .id = optCnt })); short o_##X = opt2idT[X] = optCnt++; optByUserI.push_back(X); optByUser[X] = #X;
#define optFillList(X,...) Opts.push_back(new Opt({ .id = optCnt })); short o_##X = opt2idT[X] = optCnt++; optByUserI.push_back(X); optByUser[X] = #X; \
  FOR_EACH(optFill1,__VA_ARGS__)
#define optList(X,...) optCnt = 1+PP_NARG(__VA_ARGS__); opt2Argi = new short[optCnt](); optArgi = new short[optCnt](); opt2idT = new short[optCnt](); \
  optArgi2Opt = new short[optCnt](); optidArgs = new short[optCnt](); optByUser = new LPCSTR[optCnt](); optRedefinitions = new short[optCnt](); optCnt = 0;  \
  typedef enum { X = 0, optMakeCommaList(__VA_ARGS__) } optId; optFill1(X); optFillList(__VA_ARGS__);  
#define optsMustHaveOneOf(...) optVIPs.push_back({ optMakeCommaList(__VA_ARGS__) });

enum class optRel{ unrelated, require, excludeEachOther, requireEachOther, comesBefore, comesAfter };
static const optRel optRequires = optRel::require, optUnrelated = optRel::unrelated, optExcludeEachOther = optRel::excludeEachOther,
optComesAfter = optRel::comesAfter, optRequireEachOther = optRel::requireEachOther, optComesBefore = optRel::comesBefore;
static optRel voptRel;
struct optRelation{ short opId; optRel rel = optRel::unrelated; short depOpId; LPCWSTR errMsg = nullptr; };
static vector<optRelation *> optRels;
#pragma warning (disable: 5103)
#define _2_ARGS1(id1,id2)      optRels.push_back(new optRelation({ o_##id1, voptRel, o_##id2, nullptr }));
#define _3_ARGS1(id1,id2,msg)  optRels.push_back(new optRelation({ o_##id1, voptRel, o_##id2, msg }));
#define GET_4TH_ARG1(arg1, arg2, arg3, arg4, ...) arg4
#define _MACRO_CHOOSER1(...) GET_4TH_ARG1(__VA_ARGS__, _3_ARGS1, _2_ARGS1)
#define optsRelated1(...) _MACRO_CHOOSER1 __VA_ARGS__ __VA_ARGS__
#define optsRelation(X,...) voptRel = X; FOR_EACH(optsRelated1, __VA_ARGS__)

#define OPT_ERR_REPEAT       166
#define OPT_ERR_MISSING      167
#define OPT_ERR_ARG_MISS     168
#define OPT_ERR_UNWANTED_ARG 169
#define OPT_ERR_CONFLICT     170
#define OPT_ERR_DEP_MISS     171
#define OPT_ERR_BAD_ORDER    172
#define OPT_ERR_MISUSE       173
#define OPT_ERR_USAGE        174
#define OPT_ERR_VIP_MISS     175
#define OPT_ERR_EXTRA_ARGS   176
#define OPT_ERR_ERR          177
inline int optErr(long long Err, char const* const* argv, Opt const* op1, 
  Opt const* op2 = nullptr, LPCWSTR errMsg = nullptr){
  if(errMsg){ flushOut("\n %s\n\n", *wide2uf8(errMsg)); return (int)Err; }
  #define OPTERR(I) if(!op##I){ cerr << "\n Error: internal: bad call to optErr(): op#I missing\n\n"; return OPT_ERR_ERR; }
  if(Err>=LLONG_MAX-10000){         long long i = LLONG_MAX-10000; i = Err - i;
    if(optVIPs[i].size()==1) cerr << "\n Error: required option missing : ";
    else cerr << "\n Error: at least one of these options must be provided : "; 
    for(auto n : optVIPs[i]) cerr << *((Opts[opt2idT[n]]->t)->begin()) << " "; cerr <<"\n\n";
    usage(); return OPT_ERR_VIP_MISS;
  }
  OPTERR(1);
  switch(Err){
    case OPT_ERR_REPEAT:
      cerr << "\n Error: option \""<< argv[op1->argi] <<"\" provided more than once\n\n"; usage(); break;
    case OPT_ERR_MISSING: cerr << "\n Error: option \""<< *(*op1->t).begin() <<"\" missing\n\n"; usage(); break;
    case OPT_ERR_CONFLICT:  OPTERR(2);
      cerr << "\n Error: option conflict: \""<< argv[op1->argi] <<"\" and \""<< *((*op2->t).begin()) <<"\"\n\n"; usage(); break;
    case OPT_ERR_DEP_MISS: OPTERR(2);
      cerr << "\n Error: option \""<< argv[op1->argi] <<"\" requires missing option \""<< *((*op2->t).begin()) <<"\"\n\n"; usage(); break;
    case OPT_ERR_ARG_MISS: cerr << "\n Error: missing argument for option \""<< argv[op1->argi] <<"\"\n\n"; usage(); break;
    case OPT_ERR_UNWANTED_ARG: 
      cerr << "\n Error: \""<<argv[op1->argi]<<"\" takes no argument, and \""<<argv[optidArgs[op1->id]]<<"\" not a supported option\n\n"; usage(); break;
    case OPT_ERR_BAD_ORDER:
      cerr << "\n Error: option \""<< argv[op2->argi] <<"\" shouldn't appear before \""<< argv[op1->argi] <<"\"\n\n"; usage(); break;
    case OPT_ERR_MISUSE:
      cerr << "\n Error: internal: optErr() misused.\n\n"; break;
    default: cerr << "\n Error: internal: unknown problem with provided options/arguments.\n\n"; return OPT_ERR_ERR; break;
  }
  #undef OPTERR
  return (int) Err;
}

inline int optLoad(int argc, char const* const* argv){
  string sArg; vector<short> idUsedOpts; { 
  for(auto k=0; k<optCnt; k++) if(optRedefinitions[k])    // optByUser : user of this header
    cerr <<"\n  Warning: internal: option #"<<(1+k)<<" ("<<optByUser[optByUserI[k]]<<") in call to optList() defined more than once with optAdd()\n"; }
  bool argIsAnOpt = false; short idLastOpt = 0;
  for(short i=1; i<argc; i++){
    sArg = argv[i];
    if(sArg=="-h" || sArg=="--help"){ cerr << "\n  arg #"<<i<<" : "<<sArg<<endl; usage(); return 1; }
    
    bool argIsAnOptLoc = false;
    for(short j=0; j<Opts.size(); j++){ Opt *op = Opts[j];
      if(!op->t){ cerr << "\n Error: internal: option #"<<++op->id<<" in call to optList() not defined (with optAdd())\n\n"; return OPT_ERR_MISUSE; }
      LPCSTR arg = argv[i];
      if(vectContainsStr((*(op->t)), arg)){  //cout << op->id << " : "<< arg <<"\n";
        optArgi2Opt[i] = 1+j; opt2Argi[optByUserI[op->id]] = op->argi = i; idUsedOpts.push_back(op->id);
        idLastOpt = op->id; optByUser[optByUserI[op->id]] = argv[i];  // optByUser update : now holds the opt form used by program caller
        if((op->occurences.size() >= 1) && !(op->traits & optCanRepeat))
          return optErr(OPT_ERR_REPEAT, argv, op);
        op->occurences.push_back(i); if(op->bPresence) *(op->bPresence) = true;
        argIsAnOpt = argIsAnOptLoc = true; break;
      }
    }
    if(argIsAnOptLoc) continue;

    optArgNotAnOpt.push_back(i);
    if(argIsAnOpt){  // we're in the shadow of an option
      if(std::all_of(sArg.begin(), sArg.end(), std::isspace)){ optArgNotAnOpt.pop_back(); continue; }  // skip empty args, traced in optArgNotAnOpt
      optArgNotAnOpt.pop_back(); optArgi[optByUserI[idLastOpt]] = optidArgs[idLastOpt] = i;
      argIsAnOpt = false;
    }
  }
  
  // VIP options
  for(short i=0; i<optVIPs.size(); i++){   if(optVIPs[i].size()==0) continue;
    bool b = true; for(auto id : optVIPs[i]) b = b & (opt2Argi[id]==0);
    if(b) return optErr(LLONG_MAX-10000+i, argv, nullptr);
  }
  // Mandatory options, missing args
  for(auto op : Opts){
    if((op->traits & optMandatory) && op->argi == 0)           return optErr(OPT_ERR_MISSING,  argv, op);
    if(op->argi == 0) continue;
    if((op->traits & optNeedsArg)){ if(optidArgs[op->id] == 0) return optErr(OPT_ERR_ARG_MISS, argv, op); }
    else if(optidArgs[op->id] != 0){
      if(OPT_GRACEFUL) cerr <<"\n  Warning: \""<<argv[op->argi]<<"\" takes no argument, and \""<<argv[optidArgs[op->id]]<<"\" not a supported option\n";
      else return optErr(OPT_ERR_UNWANTED_ARG, argv, op);
    }
    
  }
  // Relations between opts
  for(auto r : optRels){
    optRelation rel = *r; short id1 = rel.opId, id2 = rel.depOpId;
    bool op1 = vectContains(idUsedOpts, id1), op2 = vectContains(idUsedOpts, id2);
    #define OPTERR(err,i1,i2) return optErr(err, argv, Opts[i1], Opts[i2], rel.errMsg); break;
    switch(rel.rel){
      case optRequires:         if(op1 && !op2)	OPTERR(OPT_ERR_DEP_MISS,id1,id2);
      case optExcludeEachOther: if(op1 &&  op2) OPTERR(OPT_ERR_CONFLICT,id1,id2);
      case optRequireEachOther: if(op1 && !op2) OPTERR(OPT_ERR_DEP_MISS,id1,id2);
                                if(op2 && !op1) OPTERR(OPT_ERR_DEP_MISS,id2,id1);
      case optComesBefore: if(op1 && op2 && Opts[id1]->argi > Opts[id2]->argi) OPTERR(OPT_ERR_BAD_ORDER,id1,id2);
      case optComesAfter:  if(op1 && op2 && Opts[id1]->argi < Opts[id2]->argi) OPTERR(OPT_ERR_BAD_ORDER,id2,id1);
      default: cerr << "\n Error: internal: unknown relation specification between options \""<<(op1? *((*Opts[id1]->t).begin()) : "")
        <<"\" and \""<<(op1 ? *((*Opts[id2]->t).begin()) : "")<<"\".\n\n"; return OPT_ERR_MISUSE; break;
    }
    #undef OPTERR
  }

  return 0;
}
int optNoExtraArgs(char const* const* const& argv, bool graceful = false){
  short sz = (short)optArgNotAnOpt.size();
  if(sz==0) return 0;

  graceful |= OPT_GRACEFUL;
  if(graceful) cerr <<"\n  Warning: unused arguments:";
  else         cerr <<"\n  Error: unused arguments:";

  for(short i=0; i<sz-1; i++) cerr <<" arg#"<<optArgNotAnOpt[i]<<" \""<< argv[optArgNotAnOpt[i]]<<"\","; 
  cerr <<" arg#"<<optArgNotAnOpt[sz-1]<<" \""<<argv[optArgNotAnOpt[sz-1]]<<"\"\n";
  
  if(!graceful){ cerr <<"\n"; return OPT_ERR_EXTRA_ARGS; }
  return 0;
}

