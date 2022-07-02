// mv_tb_btn.cpp
// Copyright (c) 2022 Wasfi JAOUAD. All rights reserved.
// v0.1a 2022.06
// Versatile command-line interface to TTLib by Michael Maltsev.
// Move tarskbar buttons around (Windows 7+).


#include <windows.h>
#include <strsafe.h>

#include <propsys.h>
#include <propkey.h>
#include <shlwapi.h>
#include "TTLib/TTLib.h"

BOOL TTLib_unload_reload(bool onlyUnload);
#define clean_exit(ec) TTLib_unload_reload(true); exit(ec);
#include "utils.hpp"
#include <set>

#include <ranges>
#include <string_view>

#define MVBTN_VERSION "0.1"
using namespace std;

int usage(int rc = 0);
#include "opt.hpp"
int usage(int rc){
  cout << ""
	"\n Usage :\n"
	"\n * Between groups :"
  "\n prg.exe -cg -fg <from group label> -f <position from|0> -tg <to group label|[NEW,RAND]> [-t <position to=end|start|end>]"
  "\n prg.exe -cg -fg Notepad -f 1-4 -tg 7-Zip -t start : move buttons 1,2,3, and 4 of group Notepad to start of group 7-Zip"
  "\n prg.exe -cg -fg Notepad -f 4 -tg [NEW] : move button 4 of group Notepad to a new group named random_xxxxx"
  "\n prg.exe -cg -fg Notepad -f all -tg [RAND] : move ALL buttons of group \"Notepad\" to a new group named random_xxxxx (rename group)"
  "\n Group label can be a partial match, if it's unique. See taskbar selection in following paragraph :"
	"\n "
  "\n * Within a group : by position"
  "\n prg.exe -g <group label> -f <start position> [-t <position to=end|start|end>] [-s|-swap] [-tb <taskbar ID=0>]"
  "\n prg.exe -g Notepad -f 5 -t 2 -s : swap buttons 2 and 5 within button group \"Notepad\" in primary taskbar."
  "\n prg.exe -g Notepad -f 5 -t 2 -tb 3 : move button 5 to position 2 in group \"Notepad\" of 3rd secondary taskbar."
  "\n Taskbars are numbered : Primary = 0, Secondary = 1, 2, .. . If omitted : primary taskbar."
  "\n Arguments are case insensitive. Swap indicator : -s or -swap."
  "\n "
  "\n * Within a group: by button label"
  "\n prg.exe -g <group label> -b <button exact label> -t <position to=end|start|end> [-tb <taskbar ID=0>]"
	"\n     (button label is case sensitive)"
  "\n prg.exe -g explorer -b Computer -t 1 : move button labeled \"Computer\" to position 1 within grp \"explorer\" in primary taskbar."
  "\n"
	"\n * Position designation : "
	"\n   -f 3 : third button,   -t start : first button/start of group,  -t end : end of group"
  "\n   -f 0 or -f all : all buttons" 
	"\n   range notation: -f 4-7 : buttons 4,5,6, and 7."
	"\n   list notation: -f 9,1-3 : buttons 1,2,3 and 9."
	"\n If target position is omitted (or invalid and MVBTN_GRACEFUL=1), button is moved to end of (target) group."
	"\n When moving multiple buttons, their order before move is kept (even when repositioned in target group)."
	"\n"
  "\n Env. var. MVBTN_GRACEFUL=1 : extra arguments and unsupported options ignored."
  "\n   prg.exe -g explorer -b Computer -t 20000"
  "\n     MVBTN_GRACEFUL=1 : move button \"Computer\" to end of group explorer.exe"
  "\n     MVBTN_GRACEFUL=  : error\n"
	"\n"
	;
  return rc;
}

#include <regex>

template<typename CharType>
std::basic_string<CharType>
regexSubst(const CharType *str, const CharType *find, const CharType *rep = nullptr){
  basic_string<CharType> text(str);
  basic_regex<CharType> reg(find);
  if(rep) return regex_replace(text, reg, rep);
  else return regex_replace(text, reg, "");
}

static bool swap = false, chgGroup = false, GRACEFUL = false;
static LPWSTR group, grpFrom, grpTo, button;
bool BTN_LABEL = false, SWAP = false, NEW_GROUP = false;
static ULONG tbId = 0, iBtn1 = 0, iBtn2 = 0;
set<ULONG> iBtn1s;
static int activGrp = 0, nGroups = 0;

static vector<HANDLE> btnGrps;
static vector<int> btnCnts;
static vector<TTLIB_GROUPTYPE> btnGrpTyps;
static vector<vector<LPWSTR>> btnLabels;
static vector<vector<HWND>> btnWNHs;
static vector<LPWSTR> appIds;

static bool alpha = false;
static const bool aYes = true, aNo = false;
static const bool zeroOK = true, noZero = false;
static const bool withRanges = true, noRanges = false;
static char *gStr = nullptr;

static BOOL TTInit = FALSE, TTExplorer = FALSE, TTManip = FALSE;
bool unLoadOnly = true;

inline BOOL TTLibLoad(){
	
	if(!TTInit && TTLIB_OK!=TTLib_Init()){ cerr <<"\n Error: TTLib_Init() failed\n\n"; exit(220); }
	TTInit = TRUE;

  if(!TTExplorer && TTLIB_OK!=TTLib_LoadIntoExplorer()){ 
		cerr <<"\n Error: TTLib_LoadIntoExplorer() failed\n\n"; exit(221); }
	TTExplorer = TRUE;
  
  if(!TTManip && !TTLib_ManipulationStart()){
    cerr <<"\n Error: TTLib_ManipulationStart() failed\n\n"; exit(222);
  }
  TTExplorer = TRUE;

  return TRUE;
}

inline BOOL TTLib_unload_reload(bool onlyUnload = false){
	BOOL success = TRUE;

	if(TTManip  && !(success = TTLib_ManipulationEnd())) cerr <<"\n Error: TTLib_ManipulationEnd() failed\n";
	if(success) TTManip = FALSE;
	
	if(TTExplorer && !TTLib_UnloadFromExplorer()){
		cerr <<"\n Error: TTLib_UnloadFromExplorer() failed\n\n";
		if(TTInit) TTLib_Uninit(); exit(210);
	}; TTExplorer = FALSE;
	
	if(TTInit && !TTLib_Uninit()){ cerr <<"\n Error: TTLib_Uninit() failed\n\n";
		exit(211);
	}; TTInit = FALSE;
	
	if(success && onlyUnload) return success;
	return TTLibLoad();
}

BOOL WndSetAppId(HWND hWnd, LPCWSTR pAppId)
{
	IPropertyStore* pps;
	PROPVARIANT pv;
	HRESULT hr;

	hr = SHGetPropertyStoreForWindow(hWnd, IID_IPropertyStore, (void**)&pps);
	if (SUCCEEDED(hr))
	{
		if (pAppId)
		{
			pv.vt = VT_LPWSTR;
			hr = SHStrDup(pAppId, &pv.pwszVal);
		}
		else
			PropVariantInit(&pv);

		if (SUCCEEDED(hr))
		{
			hr = pps->SetValue(PKEY_AppUserModel_ID, pv);
			if (SUCCEEDED(hr))
				hr = pps->Commit();

			PropVariantClear(&pv);
		}

		pps->Release();
	}

	return SUCCEEDED(hr);
}

void getButtons(int k, HANDLE hTaskbar, HANDLE hButtonGroup, int nCount)
{
	vector<LPWSTR> cbtnLabels; vector<HWND> cbtnHandles;
		for(int i = 0; i < nCount; i++)
		{
			HANDLE hButton = TTLib_GetButton(hButtonGroup, i);
			HWND hWnd = TTLib_GetButtonWindow(hButton);
			cbtnHandles.push_back(hWnd);

			WCHAR szWindowTitle[MAX_APPID_LENGTH+1];
			GetWindowTextW(hWnd, szWindowTitle, MAX_APPID_LENGTH);
			cbtnLabels.push_back(*catWstr({ szWindowTitle }));
		}
		btnLabels.push_back(cbtnLabels);
		btnWNHs.push_back(cbtnHandles);
}

void getButtonGroups(HANDLE hTaskbar)
{
	HANDLE hActiveButtonGroup = TTLib_GetActiveButtonGroup(hTaskbar);

	int btnCnt;
	TTLIB_GROUPTYPE nButtonGroupType;

	if(TTLib_GetButtonGroupCount(hTaskbar, &nGroups))
	{
		for(int i = 0; i < nGroups; i++)
		{
			HANDLE hButtonGroup = TTLib_GetButtonGroup(hTaskbar, i);
			btnGrps.push_back(hButtonGroup);

			if(hButtonGroup == hActiveButtonGroup) activGrp = i;

			if(TTLib_GetButtonGroupType(hButtonGroup, &nButtonGroupType))
			  btnGrpTyps.push_back(nButtonGroupType);
			else btnGrpTyps.push_back(TTLIB_GROUPTYPE_UNKNOWN);

			WCHAR szAppId[MAX_APPID_LENGTH];
			TTLib_GetButtonGroupAppId(hButtonGroup, szAppId, MAX_APPID_LENGTH);
			appIds.push_back(*catWstr({szAppId}));

			if(TTLib_GetButtonCount(hButtonGroup, &btnCnt)) {
			  btnCnts.push_back(btnCnt);
				if(btnCnt>0) getButtons(i, hTaskbar, hButtonGroup, btnCnt);
				else{ 
					vector<LPWSTR> v; btnLabels.push_back(v); 
					vector<HWND> y; btnWNHs.push_back(y);
				}
			}
			else btnCnts.push_back(-1);
		}
	}

}

// valid target position ?
int validTargetPosition(const int grp, const int nbBtn, const int j){
	if(iBtn2==9999) iBtn2 = nbBtn;
  if(iBtn2 >(UINT) nbBtn){
    if(GRACEFUL){
      if(j==(nbBtn-1)){
        flushOut("\nOnly %d button%s in group, button #%d is already at last position :\n", nbBtn, nbBtn==1?"":"s", j+1);
        int i = 0; for(LPWSTR label : btnLabels[grp])
					flushOut("   %3d. %s\n", ++i, *wide2uf8(label));
				flushOut("Nothing to do.\n\n");
        return 1;
      } else flushOut("  Only %d button%s in group, moving button #%d to last position", nbBtn, nbBtn==1 ? "" : "s", j+1);
      iBtn2 = nbBtn; return 0;
    } else{
      flushErr("\n Error: can't move button to position %d, only %d button%s in group:\n", iBtn2, nbBtn, nbBtn==1 ? "" : "s");
      int i = 0; for(LPWSTR label : btnLabels[grp])
        flushErr("   %3d. %s\n", ++i, *wide2uf8(label));
      flushErr("\nAbort.\n\n");
      return 2;
    }
  }
  if(iBtn2==nbBtn){
    flushOut("Group has %d button%s, and button #%d is already at last position. Nothing to do.\n\n", nbBtn, nbBtn==1 ? "" : "s", j+1);
    return 1;
  }
	flushOut("    Moving button \"%s\"", *wide2uf8(btnLabels[grp][j]));
	return 0;
}

int groupByLabel(LPCWSTR mGroup){

	vector<int> grpMatch; vector<LPWSTR> mpos; //LPWSTR *mpos = new LPWSTR[(int) appIds.size()]();
	LPWSTR p; int k = 0;

	for(int i = 0; i < nGroups; i++)
		if(NULL!=(p = StrStrIW(appIds[i], mGroup))){ grpMatch.push_back(i); mpos.push_back(p); }
	if(grpMatch.size() == 0){
		flushErr("\n Error: no group labeled \"%s\"\n\nAbort.\n\n", *wide2uf8(mGroup));
		return -1;
	}
	if(grpMatch.size()>1){
		flushErr("\n Error: multiple matches for group label \"%s\" :\n", *wide2uf8(mGroup));
		for(short i=0; i<grpMatch.size(); i++){
			k = grpMatch[i];
			flushErr("   group #%d: %s\n%*s^\n", k, *wide2uf8(appIds[k]), _snprintf(NULL, 0, "   group #%d: ", k)+(mpos[i]-appIds[k]),"");
		}
		flushErr("Abort.\n\n");
		return -1;
	}
	int grpId = grpMatch[0];

	if(btnCnts[grpId] <= 0){
		flushErr("\n Error: group #%d: %s\n has no buttons !!\nAbort.\n\n", grpId+1, *wide2uf8(appIds[grpId]));
		return -1;
	}
	int cnt = btnCnts[grpId];
	if(0==lstrcmpW(mGroup, appIds[grpId]))
		flushOut("      group \"%s\" (#%d, %d button%s)\n", *wide2uf8(mGroup), grpId+1, cnt, cnt==1?"":"s");
	else flushOut("      \"%s\" matches: %s (grp #%d, %d button%s)\n", *wide2uf8(mGroup), *wide2uf8(appIds[grpId]), grpId+1, cnt, cnt==1?"":"s");
	return grpId;
}

BOOL mvTaskbarButtons(HANDLE hTaskbar){

	int grpId = groupByLabel(group); if(grpId<0) return FALSE;
	group = appIds[grpId];
  int nbButtons = (int) (btnLabels[grpId]).size();
	if(iBtn1==9999) iBtn1 = nbButtons;
  
	// -g <group label> -b <button exact label> -t <position to=end|start|end>
	if(BTN_LABEL){
		// Locate button
		int j = -1;
    for(int i = 0; i < nbButtons; i++)
      if(0==lstrcmpW(btnLabels[grpId][i], button)){ j = i; break; }
    if(j < 0){
      int i = 0;
      flushErr("\n Error: group #%d: %s\n has no button labeled : %s\n\n  Buttons:\n", grpId, *wide2uf8(appIds[grpId]), *wide2uf8(button));
      for(LPWSTR label : btnLabels[grpId])
        flushErr("   %3d. %s\n", ++i, *wide2uf8(label));
      flushErr("\nAbort.\n\n");
      return FALSE;
    }
		flushOut("      Matching button in group: #%d\n", j+1);
		
		int rc;  if(2==(rc = validTargetPosition(grpId, nbButtons, j))) return FALSE;
		if(rc==1) return TRUE;

		if(TTLib_ButtonMoveInButtonGroup(btnGrps[grpId], j, iBtn2 - 1))	flushOut(" .. done\n\n");
		else{ flushErr("\n\n Error: operation failed !\n\n"); return FALSE; }

		return TRUE;
	}  // end BTN_LABEL

	// -g <group label> -f <start position>[-t <position to=end|start|end>][-s|-swap]
	// Button to move exists ?
  if(iBtn1>(UINT) nbButtons){
    flushErr("\n Error: no button #%d, only %d button%s in group !\n", iBtn1, nbButtons, nbButtons==1 ? "" : "s");
    int i = 0; for(LPWSTR label : btnLabels[grpId])
      flushErr("   %3d. %s\n", ++i, *wide2uf8(label));
    flushErr("\nAbort.\n\n");
    return FALSE;
  }

	if(!SWAP){
    int rc;  if(2==(rc = validTargetPosition(grpId, nbButtons, iBtn1-1))) return FALSE;
    if(rc==1) return TRUE;

    if(TTLib_ButtonMoveInButtonGroup(btnGrps[grpId], iBtn1-1, iBtn2-1)) flushOut(" .. done\n\n");
    else{ flushErr("\n\n Error: operation failed !\n\n"); return FALSE; }

		return TRUE;
	}

  // Button to swap exists ?
  if(iBtn2>(UINT) nbButtons){
    flushErr("\n Error: no button #%d, only %d button%s in group !\n", iBtn2, nbButtons, nbButtons==1 ? "" : "s");
    int i = 0; for(LPWSTR label : btnLabels[grpId])
      flushErr("   %d. %s\n", ++i, *wide2uf8(label));
    flushErr("\nAbort.\n\n");
    return FALSE;
  }

	ULONG iBtn11 = iBtn1, iBtn22 = iBtn2;
	iBtn2 < iBtn1 ? (iBtn22 = iBtn1) & (iBtn11 = iBtn2) : true;

	flushOut("    Moving button #%lu (%s) to position %lu", iBtn22, *wide2uf8(btnLabels[grpId][iBtn22-1]), iBtn11);
	if(TTLib_ButtonMoveInButtonGroup(btnGrps[grpId], iBtn22-1, iBtn11-1)) flushOut(" .. done\n");
  else{ flushErr("\n\n Error: operation failed !\n\n"); return FALSE; }

	if(iBtn11==(iBtn22-1)){
		flushOut("    Swap done, button #%lu (%s) now in position %lu\n\n", iBtn11, *wide2uf8(btnLabels[grpId][iBtn11-1]), iBtn22);
		return TRUE;
	}

	flushOut("    Moving button #%lu (%s) to position %lu", iBtn11+1, *wide2uf8(btnLabels[grpId][iBtn11-1]), iBtn22);
  if(TTLib_ButtonMoveInButtonGroup(btnGrps[grpId], iBtn11, iBtn22-1)) flushOut(" .. done\n    Buttons swapped.\n\n");
  else{ flushErr("\n\n Error: operation failed !\n\n"); return FALSE; }

	return TRUE;
}

BOOL mvTaskbarButtonsGr(HANDLE hTaskbar){

	// -cg <from group label> -f <position from|0> -tg <to group label|[NEW] or [RAND]> [-t <position to=end|start|end>]
  int grpId = groupByLabel(grpFrom); if(grpId<0) return FALSE;
	grpFrom = appIds[grpId];

  int n = btnCnts[grpId];
	if(n<=0){
		flushErr("\n Error: group #%d: %s\n has no buttons !!\nAbort.\n\n", grpId+1, *wide2uf8(appIds[grpId]));
		return FALSE;
	}
	UINT nbButtons = (UINT)n;

	UINT nbBtns1 = (UINT) iBtn1s.size();
	if(iBtn1 == 9999) iBtn1 = nbButtons;
  if(nbBtns1 > nbButtons || ( iBtn1>0 && iBtn1 > nbButtons )){
    flushErr("\n Error: move button #%d : group #%d has only %d button%s !\nAbort.\n\n", nbBtns1>iBtn1? nbBtns1 : iBtn1, grpId+1, nbButtons, nbButtons==1?"":"s");
    return FALSE;
  }

  //#define chkCallRet(X,F) { X = F;	if(!X) return FALSE; }

	if(NEW_GROUP){
		flushOut("      Random new group: %s\n", *wide2uf8(grpTo));
		bool grToExists = false, really = false; for(auto gr : appIds) if(really = grToExists = (0==lstrcmpW(gr, grpTo))) break;
		UINT i = 0;  while(grToExists && ++i<=100){
			grpTo = *uf8toWide(*catStr({ "random_", random_string(2+(i<11?i:10), true).c_str() }));
			flushOut("        a group with that name exists already ! New random name: ", *wide2uf8(grpTo));
			grToExists = false; for(auto gr : appIds) if(grToExists = (0==lstrcmpW(gr, grpTo))) break;
		}; if(really && !grToExists) flushOut("        ok, no such group exists, using this name.");
		if(really && grToExists){ flushOut("\n Error: could not generate a random group name that is not already in use !!\n\n"); return FALSE; }
		
		if(iBtn1==0){ // mv all buttons
			flushOut("  Moving buttons");
			for(i = 0; i < nbButtons; i++)
				if(!WndSetAppId(btnWNHs[grpId][i], grpTo)){ flushErr("\n\n Error: operation failed !\n\n"); return FALSE; }
			flushOut(" .. done\n\n");

			if(!TTLib_unload_reload()) return FALSE;  // TTLib_ManipulationEnd() failed ?

      if(!TTLib_ButtonGroupMove(hTaskbar, (UINT)appIds.size(), grpId)){   // or add TTLib_Init/Uninit and use appIds.size()-1
		    flushErr("\n Error: failed to move new group to position %lu\n\n", grpId+1); return FALSE; }
			return TRUE;
		} 
		else{
			flushOut("  Moving button%s to new group", nbBtns1?"s":"");
			if(!nbBtns1) iBtn1s.insert(iBtn1);
			for(auto btn : iBtn1s)
        if(!WndSetAppId(btnWNHs[grpId][btn-1], grpTo)){ flushErr("\n\n Error: operation failed !\n\n"); return FALSE; }
      flushOut(" .. done\n\n");
			return TRUE;
		}
	}	 // end NEW_GROUP
	else { 
		
		int grpId2 = groupByLabel(grpTo); if(grpId2<0) return FALSE;
		grpTo = appIds[grpId2];
		n = btnCnts[grpId2];
    if(n<=0){
      flushErr("\n Error: group #%d: %s\n has no buttons !!\nAbort.\n\n", grpId2+1, *wide2uf8(appIds[grpId2]));
      return FALSE;
    }
		UINT nbButtons2 = (UINT) n;
    if(iBtn2 == 9999) iBtn2 = 1+nbButtons2;
    if(iBtn2>0 && iBtn2 > nbButtons2){
      flushErr("\n Error: group #%d has only %d button%s !\nAbort.\n\n", grpId2+1, nbButtons2, nbButtons2==1 ? "" : "s");
      return FALSE;
    }
		
    if(!nbBtns1 && iBtn1==0){ // mv all buttons
			flushOut("\n  Moving button%s to end of group \"%s\"", nbButtons==1?"":"s", *wide2uf8(grpTo));
			for(UINT i = 0; i < nbButtons; i++)
				if(!WndSetAppId(btnWNHs[grpId][i], grpTo)){ flushErr("\n\n Error: operation failed !\n\n"); return FALSE; }
			flushOut(" .. done\n");
			if(iBtn2==(1+nbButtons2)){ flushOut("\n"); return TRUE; }

      flushOut("  Moving button%s to position %lu within \"%s\"", nbButtons==1?"":"s", iBtn2, *wide2uf8(grpTo));

			if(!TTLib_unload_reload()) return FALSE;  // TTLib_ManipulationEnd() failed ?
			
			int j = 0;
			for(UINT i = nbButtons2; i < nbButtons2+nbButtons; i++)				//cout << (i+1)<<" -> " <<iBtn2+(j++) <<"\n";
        if(!TTLib_ButtonMoveInButtonGroup(btnGrps[grpId2], i, iBtn2+(j++)-1)){
          flushErr("\n\n Error: operation failed\n\n"); return FALSE;
        }
      flushOut(" .. done\n\n"); return TRUE;

			return TRUE;
    }
		else{
			if(nbBtns1) flushOut("  Moving button%s to end of group \"%s\"", nbBtns1?"s":"", *wide2uf8(grpTo));
			else{
				iBtn1s.insert(iBtn1);
				flushOut("  Moving button #%lu to end of group \"%s\"", iBtn1, *wide2uf8(grpTo));
			}
      for(auto btn : iBtn1s)
			  if(!WndSetAppId(btnWNHs[grpId][btn-1], grpTo)){ flushErr("\n\n Error: operation failed !\n\n"); return FALSE; }
			flushOut(" .. done\n");
			if(iBtn2==(1+nbButtons2)){ flushOut("\n"); return TRUE; }
			
			flushOut("  Moving button%s to position %lu within \"%s\"", nbBtns1?"s":"", iBtn2, *wide2uf8(grpTo));

			if(!TTLib_unload_reload()) return FALSE;
			
      int j = 0;
      for(UINT i = nbButtons2; i < nbButtons2+nbBtns1; i++)				//cout << (i+1)<<" -> " <<iBtn2+(j++) <<"\n";
        if(!TTLib_ButtonMoveInButtonGroup(btnGrps[grpId2], i, iBtn2+(j++)-1)){
          flushErr("\n\n Error: operation failed\n\n"); return FALSE;
        }

			flushOut(" .. done\n\n"); return TRUE;
		}
	}

  #undef chkCallRet
	
	return FALSE;

}

BOOL mvButtons(HANDLE hTaskbar)
{
	getButtonGroups(hTaskbar);
	if(!chgGroup) return mvTaskbarButtons(hTaskbar);
	return mvTaskbarButtonsGr(hTaskbar);
	return TRUE;
}

void allocFail() {
	cerr << "\n Error: memory allocation failure, aborting.\n";
	TTLib_unload_reload(unLoadOnly);
	exit(55);  //set_new_handler(nullptr);
}

int processArgs(int argc, char const* const* const& argv, LPWSTR const* const& arglist);
int main(int argc, char **argv)
{
	BOOL bSuccess = FALSE;

	setlocale(LC_ALL, "en-US.65001");
	set_new_handler(allocFail);
 
  int nbArgs;
	LPWSTR *arglist = CommandLineToArgvW(GetCommandLineW(), &nbArgs);
  if(arglist == nullptr) printErr(nullptr, sysErr, 21);

	{ LPWSTR val; if(getEnvVar(L"MVBTN_GRACEFUL", val) && 0==lstrcmpW(val, L"1")) GRACEFUL = true; }

	int rc = processArgs(argc,argv, arglist); if(rc==200) return 0; if(rc!=0) return rc;

		//flushOut("Loading 7+ Taskbar Tweaking library into explorer...\n");
	{ TTLibLoad();

      HANDLE hTaskbar = TTLib_GetMainTaskbar();
      if(tbId==0) bSuccess = mvButtons(hTaskbar);
			else{
				int nCount;
				if(TTLib_GetSecondaryTaskbarCount(&nCount)){
					if(tbId > (UINT) nCount){
						flushOut("\n  Error: secondary taskbar #%lu : ", tbId);
						if(nCount>0) flushOut("only %d secondary taskbar%s found.\n\n", tbId, nCount, nCount>1 ? "s" : "");
						else flushOut("no secondary taskbars found, only a primary taskbar.\n\n");
						return FALSE;
					}
					hTaskbar = TTLib_GetSecondaryTaskbar(tbId);
					bSuccess = mvButtons(hTaskbar);
				}
			}

	  TTLib_unload_reload(unLoadOnly);
	}
	LocalFree(arglist);
	return bSuccess ? 0 : 1;
}

int checkNbr(short op, long long &i, char const* const* const& argv, LPWSTR const* const& arglist,short okZero);
int checkNbrc(short op, char const *const &nbr, long long &i, short okZero);
int processRanges(short op, char const *const *const &argv, set<ULONG> &set, short okZero, bool noRanges);

int processArgs(int argc, char const* const* const& argv, LPWSTR const* const& arglist){

  int nbArgs = argc - 1;
	bool posFrom = false, posTo = false, tbar = false;

	// -cg -fg <from group label> -tg <to group label|[NEW:label]|[NEW] or[RAND]> -f <position from|0> -t [position to=end|start|end]
		// -g <group label> -f <start position> -t <target position> -[s|swap]]
		// -g <group label> -b <button exact label> -t <position> 
		// -tb [taskbar ID=0]
	if(argc==2){ string opt(argv[1]); if(opt=="-h" || opt=="-help"){ usage(); return 200; } }  // quick exit
  if(argc < 4){ cerr << "\n  Error: not enough arguments\n\n";	usage(1); return 1; }

	OPT_GRACEFUL = GRACEFUL;
  optsNeedArgByDefault = true;

	optList(tb, cg, fg, tg, f, t, g, b, s);
  optAdd(
		( cg, ("-cg", "--change-group"), optHasNoArg           ),
		( fg, ("-fg", "--from-group")                          ),
		( tg, ("-tg", "--target-group")                        ),
		( f,  ("-f", "--from", "-from")                        ),
		( t,  ("-t", "--to", "-to")                            ),
		( g,  ("-g", "--group", "-group")                      ),
		( b,  ("-b", "--button", "-button")                    ),
		( s,  ("-s", "--swap", "-swap"), optHasNoArg           ),
    ( tb, ("-tb", "--taskbar", "-taskbar"), optCanRepeat   ) 
	);
  
	optsMustHaveOneOf(b,f); 

  optsRelation( optExcludeEachOther, (cg, g), (cg, b), (cg, s), (b, s),
    (b, f, L"Error: either designate button to move by label (-b) or by position (-f), not both"));
	optsRelation( optRequireEachOther, (cg,fg), (cg,tg) );
	optsRelation( optRequires,         (cg,f),  (g,t), (b,g) );


	optSetIndicator( (cg, chgGroup), (f, posFrom), (t, posTo), (b, BTN_LABEL), (tb, tbar), (s, SWAP) );
	
	int rc;
	#define chkCallRet(X) rc = X;	if(rc!=0) return rc;
	chkCallRet( optLoad(argc, argv) );
	chkCallRet( optNoExtraArgs(argv) );

	long long i; //optId userOpt;
	#define checkGetNbr(opt,i,bZero) chkCallRet( checkNbr(opt, i, argv,arglist, bZero) )
  #define checkGetPureNbr(opt,var,bZero) { checkGetNbr(opt,i,bZero); var = (ULONG) i; }
  #define checkGetArgAsNbr(opt,var,bZero) { if(optArgi[opt]) {                     \
         if(StrStrIW(arglist[optArgi[opt]], L"start"))             var = 1;        \
    else if(StrStrIW(arglist[optArgi[opt]], L"end"))               var = 9999;     \
    else if(bZero && (0==StrCmpIW(arglist[optArgi[opt]], L"0")                     \
	                 || 0==StrCmpIW(arglist[optArgi[opt]], L"All"))) var = 0;        \
    else checkGetPureNbr(opt,var,bZero); }}

	if(tbar) checkGetArgAsNbr(tb, tbId, zeroOK);

	if(chgGroup){
		if(nbArgs <= 3){ cerr << "\n  Error: not enough arguments for "<<optByUser[cg]<<" mode.\n\n"; usage(1); return OPT_ERR_USAGE; }
		// -cg     -fg <from group label>    -tg <to group label|[NEW] or [RAND]>     -f <position from|[0, All]|start|end>       [-t <position to=end|start|end>]
		grpFrom = arglist[optArgi[fg]]; grpTo = arglist[optArgi[tg]]; LPCSTR ng = *wide2uf8(*catWstr({ L"group \"", grpTo, L"\""}));
		if(StrStrIW(grpTo, grpFrom)){ cerr <<"\n Error: source and target group are the same. Please use -g to move buttons within a group.\n\n"; return 100; }
		if(0==lstrcmpiW(grpTo, L"[NEW]") || 0==lstrcmpiW(grpTo, L"[RAND]")){
			grpTo = *uf8toWide(*catStr({ "random_", random_string(2,true).c_str() })); NEW_GROUP = true;  ng = "a new group"; }
    
		rc = processRanges(f, argv, iBtn1s, noZero, withRanges);
		switch(rc){
			case 0:         // all good			
			case 1: break;  // list with 0
			// no list :
			case 2: checkGetArgAsNbr(f, iBtn1, zeroOK); break;
			// Unauthorized zero :
      case 3: flushErr("\n  Error: in argument to \"%s\": \"%s\" : 0 means all buttons, cannot be with other button positions\n    "
				"If you mean \"all buttons\", simply use : \"-f 0\", or \"-f all\"\n\n", optByUser[f], gStr); return 31;
			// malformed list :
			default: return (200+rc);
		}
		if(iBtn1s.size() == 1){ iBtn1 = *iBtn1s.begin(); iBtn1s.clear(); }

		if(posTo) checkGetArgAsNbr(t, iBtn2, zeroOK) else iBtn2 = 9999;

		flushOut("\n Action: move "); char *gf = *wide2uf8(grpFrom), *gt = *wide2uf8(grpTo);
		char *btnfrom = new char[(size_t) 100+(rc=snprintf(NULL, 0, "%lu", iBtn1))]();
		iBtn1s.size() > 0 ? snprintf(btnfrom, (size_t)(100+rc), "%d %s", (int)iBtn1s.size(), "buttons") : snprintf(btnfrom, (size_t)(100+rc), "button #%lu", iBtn1);
		if(iBtn2==9999){
			if(iBtn1>0 || iBtn1s.size() > 1) flushOut("%s in group \"%s\" to %s%s", btnfrom, gf, NEW_GROUP?"":"end of ", ng);
			else flushOut("all buttons in group \"%s\" to end of %s \"%s\" ", gf, ng, gt);
		} else{
      if(iBtn1>0 || iBtn1s.size() > 0) flushOut("%s in group \"%s\" to position %lu in %s", btnfrom, gf, iBtn2, ng);
      else flushOut("all buttons in group \"%s\" to position %lu in %s", gf, iBtn2, ng);
		}
		if(tbId == 0) flushOut(" (primary taskbar)\n"); else flushOut(" (secondary taskbar #%lu)\n", tbId);
		return 0;
	} // chgGroup
	
	group = arglist[optArgi[g]]; char *gr = *wide2uf8(group);
  // mv_btn.exe -g <group label> -f <start position=end|start|end>     -t <target position=end|start|end>     [-tb <taskbar ID=0>   [-swap]]
  // mv_btn.exe -g <group label> -b <button exact label>               -t <target position=end|start|end>     [-tb <taskbar ID=0>]
	if(BTN_LABEL) button = arglist[optArgi[b]];
	else if(SWAP){ 
		if(regexSubst(argv[optArgi[f]], "[0-9,-]").size()==0 && regexSubst(argv[optArgi[f]], "[^,-]").size()>0){
		  flushErr("\n Error: in argument to \"%s\": \"%s\" : cannot use list/range format with %s.\n Try option -h\n\n", optByUser[f], argv[optArgi[f]], optByUser[s]); 
			return 32;
		}
		checkGetArgAsNbr(f, iBtn1, noZero)
	} 
	else {		
    rc = processRanges(f, argv, iBtn1s, noZero, withRanges);
    switch(rc){
      case 0:         // all good
      case 1: break;  // list with 0
      // no list :
      case 2: checkGetArgAsNbr(f, iBtn1, noZero); break;
      // Unauthorized zero :
      case 3: flushErr("\n  Error: in argument to \"%s\": \"%s\" : 0 means all buttons, cannot be with other button positions\n    "
        "If you mean \"all buttons\", simply use : \"-f 0\", or \"-f all\"\n\n", optByUser[f], gStr); return 31;
      // malformed list :
      default: return (200+rc);
    }
	}
	if(iBtn1s.size() == 1){ iBtn1 = *iBtn1s.begin(); iBtn1s.clear(); }
	
  if(posTo && regexSubst(argv[optArgi[t]], "[0-9,-]").size()==0 && regexSubst(argv[optArgi[t]], "[^,-]").size()>0){
    flushErr("\n Error: in argument to \"%s\": \"%s\" : you cannot use list/range format for target.\n Try option -h\n\n", optByUser[t], argv[optArgi[t]]);
    return 32;
  }
	if(posTo) checkGetArgAsNbr(t, iBtn2, noZero) else iBtn2 = 9999;

	if(iBtn1==iBtn2){ flushOut("\n Source and target positions are the same. Nothing to.\n\n", iBtn1, gr); return 200; }
  
	if(BTN_LABEL){           char *btn = *wide2uf8(button);
		if(iBtn2==9999) flushOut("\n Action: move button \"%s\" in group \"%s\" to last position", btn, gr);
		else flushOut("\n Action: move button \"%s\" in group \"%s\" to position %lu", btn, gr, iBtn2);
	} else {  // no btn label
    char *btnfrom = new char[(size_t) 100+(rc=snprintf(NULL, 0, "%lu", iBtn1))]();
    iBtn1s.size() > 0 ? snprintf(btnfrom, (size_t) (100+rc), "%d %s", (int) iBtn1s.size(), "buttons") : snprintf(btnfrom, (size_t) (100+rc), "button #%lu", iBtn1);
    if(SWAP){ if(iBtn2==9999) flushOut("\n Action: swap %s with last button in group \"%s\" ", btnfrom, gr);
		          else {
			          if(iBtn1==9999) flushOut("\n Action: swap last button with button at position %lu in group \"%s\"", iBtn2, gr);
								else flushOut("\n Action: swap buttons #%lu and #%lu in group \"%s\"", iBtn1, iBtn2, gr);
							}
	  } else {  // no swap
			if(iBtn2==9999) flushOut("\n Action: move %s in group \"%s\" to last position", btnfrom, gr);
			else{
				if(iBtn1==9999) flushOut("\n Action: move last button in group \"%s\" to position %lu", gr, iBtn2);
				else flushOut("\n Action: move %s in group \"%s\" to position %lu", btnfrom, gr, iBtn2);
			}
	}}
  if(tbId == 0) flushOut(" (primary taskbar)\n"); else flushOut(" (secondary taskbar #%lu)\n", tbId);

	#undef checkGetArgNbr
	#undef checkGetNbr
	#undef chkCallRet
	return 0;
}

inline int checkNbr(short op, long long &i, char const* const* const& argv, LPWSTR const* const& arglist, short okZero = noZero){
	short iArg = optArgi[op]; const char *arg = argv[iArg], *opt = optByUser[op];
	string cs(arg); static string numchars = " +-0123456789";
		for(char c : numchars) cs.erase(remove(cs.begin(), cs.end(), c), cs.end());
	LPCWSTR argW = arglist[iArg];
  if(cs.size() != 0){ flushErr("\n Error: arg%d (to option \"%s\") : expecting a number, not '%s'\n Try option -h\n\n",iArg, opt,arg); return 24; }
  char *end = nullptr; errno = 0; i = strtoll(arg, &end, 10);
  if(errno == ERANGE){ flushErr("\n Error: arg%d: '%s' : ",iArg,arg); perror(""); flushErr("\n Try option -h\n\n"); return 25; }
  if(okZero==noZero && i==0){ flushErr("\n Error: arg%d (to option \"%s\") : expecting a positive number, not zero. Try option -h\n\n",iArg, opt); return 26; }
	if(i < 0){ flushErr("\n Error: arg%d (to option \"%s\") : expecting a positive number, not '%s'\n Try option -h\n\n",iArg, opt,arg); return 27; }
  if(end && 0 < strlen(end)){ flushErr("\n Error: arg%d: expecting a positive number, not '%s'.\n Try option -h\n\n", iArg, *wide2uf8(argW)); return 28; }
  return 0;
}

inline int checkNbrc(short op, char const* const& nbr, long long &i, short okZero = noZero){
	short iArg = optArgi[op]; static string chars = " "; string cs(nbr);
	for(char c : chars) cs.erase(remove(cs.begin(), cs.end(), c), cs.end());
  char *end = nullptr; errno = 0; i = strtoll(regexSubst(nbr, "\\s+").c_str(), &end, 10);
  if(errno == ERANGE){ flushErr("\n Error: arg%d: '%s' : ",iArg,nbr); perror(""); flushErr("\n Try option -h\n\n"); return 25; }
  if(i < 0){ flushErr("\n Error : arg%d : expecting a positive number, not '%s'\n Try option -h\n\n",iArg,nbr); return 27; }
  if(end && 0 < strlen(end)){ flushErr("\n Error: arg%d: expecting a positive number, not '%s'\n Try option -h\n\n", iArg, nbr); return 28; }
  return 0;
}

inline int processRanges(short op, char const* const* const& argv, set<ULONG>& set, short okZero = zeroOK, bool Ranges = false){
	
	const char *arg = argv[optArgi[op]];
	static bool zeroIntheList = false;
	
  if(!StrStrIA(arg, ",") && !regex_match(arg, regex("^[ ]*[0-9]+[ ]*-[ ]*[0-9]+[ ]*$", regex_constants::egrep)))
		return 2;  // not a list

  if(!Ranges && !regex_match(arg, regex("^[ ]*([0-9]+[ ]*,[ ]*)+[0-9]+[ ]*$", regex_constants::egrep))){
    flushErr("\n  Error: not a comma-separated list of numbers: %s\n\n", regexSubst(arg, "^\\s+"));
    return 10;
  }

	if(Ranges && !regex_match(arg, regex("^[ ]*([0-9]+[ ]*(-[ ]*[0-9]+)*[ ]*,[ ]*)+[0-9]+[ ]*(-[ ]*[0-9]+)[ ]*$|^[ ]*[0-9]+[ ]*-[ ]*[0-9]+[ ]*$",
		regex_constants::egrep)))
	{
		flushErr("\n  Error: malformed list of numbers/ranges: %s\n\n", regexSubst(arg, "^\\s+"));
		return 10;
	}
	
  #define splitView(c) ranges::views::split(c) \
			| ranges::views::transform([](auto &&str){ return string_view(&*str.begin(), ranges::distance(str)); })

	long long i; int rc;

  auto split = string(arg) | splitView(',');
	for(auto &&word : split){

		if(noRanges){  // taskbars
      if(0 != (rc = checkNbrc(op, string(word).c_str(), i, zeroOK))) return rc;
			if(!okZero && i==0){ gStr = *catStr({ regexSubst(word.data(), "\\s+").c_str() }); return 3; }
			if(i==0) zeroIntheList = true;
      set.insert((ULONG) i);
			continue;
		}

		// word is a range (buttons only)
		if(regex_match(string(word), regex("^[ ]*[0-9]+-[0-9]+[ ]*$", std::regex_constants::egrep))){
			auto splitw = string(word) | splitView('-');
			vector<long long> nbrs; auto iter = begin(splitw); auto pastLastElem = end(splitw);
			for(auto &&nbr : splitw){
				if(0 != (rc = checkNbrc(op, string(nbr).c_str(), i, zeroOK))) return rc;
				if(!okZero && i==0){ gStr = *catStr({ regexSubst(word.data(), "\\s+").c_str() }); return 3; }
				if(i==0) zeroIntheList = true;
				nbrs.push_back(i-1);
				if(++iter==pastLastElem){
					if(nbrs[0]>nbrs[1]){
						flushErr("\n  Error: in argument to \"%s\": \"%s\": not a valid numeric range (%d > %d, did you mean "
							"%d-%d ?)\n\n", optByUser[op], regexSubst(word.data(), "\\s+"), 1+nbrs[0], 1+nbrs[1], 1+nbrs[1], 1+nbrs[0]); return 30;
					}
					for(i=nbrs[0]; (i++)<=nbrs[1];) set.insert((ULONG) i);
				}
			}
    } // end range
		else {  // word is a single nbr
			if(0 != (rc = checkNbrc(op, string(word).c_str(), i, zeroOK))) return rc;
			if(!okZero && i==0){ gStr = *catStr({ regexSubst(word.data(), "\\s+").c_str() }); return 3; }
			if(i==0) zeroIntheList = true;
			set.insert((ULONG) i);
    }
  }
	#undef splitView
	return zeroIntheList ? 1:0;
}
