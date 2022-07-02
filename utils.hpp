// utils.hpp
// Copyright (c) 2022 Wasfi JAOUAD. All rights reserved.
// v1.8 2022.06
// Misc little routines.

// #define clean_exit(ec) before including to go through your cleaner on the fatal way out, or :
// #define clean_exit(ec) exit(ec)


//#include <windows.h>
#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>
#include <sstream>
#include <random>

using namespace std;
#define sysErr 10000+GetLastError()

void printErr(LPCSTR msg, LONG errCode, int exitCode);
unique_ptr<LPSTR> wide2uf8(LPCWSTR str);

inline void chkAlloc(size_t count) {};
template <typename T, class ... Ts>
inline void chkAlloc(size_t count, T*& x, Ts&& ...args) {
	try { x = new T[count](); }
	catch (const bad_alloc) {
		fprintf(stderr, "Error allocating memory (%zu x %zu bytes).\n\n", count, sizeof(T));
    clean_exit(15);
	}
	chkAlloc(count, args...);
}

template <typename T>
inline void chkRealloc(T*& x, size_t count) {
  if(x==nullptr) printErr("Error: chkRealloc(): cannot be realloc null pointer\n", 0, 15);
  x = (T*)realloc((void*)x, count * sizeof(T));
  if (nullptr == x) {
    fprintf(stderr, "\n  Error: not enough memory (trying to reserve %zu x %zu bytes).\n\n", count, sizeof(T)); clean_exit(14);;
  }
}

void checkedVectResz(size_t count){}
template <typename T, class ... Ts>
void checkedVectResz(size_t count, vector<T> &x, Ts&& ...args){
  try{ x.resize(count); } catch(const bad_alloc){
    fprintf(stderr, "Error allocating memory (%zu x %zu bytes).\n\n", count, sizeof(T));
    clean_exit(14);
  }
  checkedVectResz(count, args...);
}

inline void trim(string& str) {
	if (str.empty()) return;
	const auto pStr = str.c_str();
	size_t front = 0, back = str.length();
	while (front < back && isspace(static_cast<unsigned char>(pStr[front]))) ++front;
	while (back > front && isspace(static_cast<unsigned char>(pStr[back - 1]))) --back;

	if (0 == front) { if (back < str.length()) str.resize(back - front); return; }
	if (back <= front) str.clear();
	else str = move(string(str.begin() + front, str.begin() + back));
}
bool iequals(const string& a, const string& b)
{
	return equal(a.begin(), a.end(),
		b.begin(), b.end(),
		[](char a, char b) {
			return tolower(a) == tolower(b);
		});
}

inline void flushErr(LPCSTR format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args); fflush(stderr);
	va_end(args);
}
inline void flushOut(LPCSTR format, ...) {
	va_list args;
	va_start(args, format);
	vfprintf(stdout, format, args); fflush(stdout);
	va_end(args);
}

inline void printErr(LPCSTR msg, LONG errCode, int exitCode) {
	if (errCode > 10000) {  // query system
		errCode -= 10000;
		if (nullptr != msg) flushErr("\n  %s\n", msg);
		LPWSTR messageBuffer = nullptr;
		if (0 < FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, (DWORD)errCode, 0, (LPWSTR)&messageBuffer, 0, nullptr)) {
			LPSTR u8str = *wide2uf8(messageBuffer);
			size_t n = strlen(u8str) - 1; while (u8str[n] == '\n') u8str[(n--)] = 0;  // thank you
			flushErr("  (Err %d) %s\n\n", errCode, u8str);
			LocalFree(messageBuffer); delete[] u8str; //free((void *)u8str);
		}
		if (exitCode != 0) clean_exit(exitCode);
		return;
	}
	if (nullptr != msg) flushErr("\n  %s\n", msg);
	if (exitCode != 0) clean_exit(exitCode);
}

inline unique_ptr<LPSTR> wide2uf8(LPCWSTR str) {
	auto dwCount = WideCharToMultiByte(CP_UTF8, 0, str, -1, nullptr, 0, nullptr, nullptr);
	if (0 == dwCount) {
		DWORD errorID = GetLastError();
		fprintf(stderr, "Error: wide2uf8(): WideCharToMultiByte() failed.\n"); fflush(stderr);
		printErr(nullptr, 10000 + errorID, 57);
	}
	char* pText = nullptr; chkAlloc(dwCount, pText);
	if (0 == WideCharToMultiByte(CP_UTF8, 0, str, -1, pText, dwCount, nullptr, nullptr))
		printErr("Error: uf8toWide(): WideCharToMultiByte() failed\n", 10000 + GetLastError(), 58);
	return make_unique<LPSTR>(pText);
}

inline unique_ptr<LPWSTR> uf8toWide(LPCCH str) {
  auto dwCount = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, nullptr, 0);
  if (0==dwCount){ DWORD errorMessageID = GetLastError();
    fprintf(stderr, "Error: MultiByteToWideChar() failed: %s\n",str); fflush(stderr);
    printErr(nullptr, 10000+errorMessageID, 59); }
  wchar_t *pText = nullptr; chkAlloc(dwCount, pText);
  if(0==MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, pText, dwCount))
    printErr("Error: uf8toWide(): MultiByteToWideChar() failed\n", sysErr, 55);
  return make_unique<LPWSTR>(pText);
}

inline unique_ptr<LPSTR> catStr(initializer_list<LPCSTR> list){
  size_t sz = 0;
  HRESULT hRslt = S_OK;
  LPSTR buff; chkAlloc(1, buff); buff[0] = '\0';
  for(auto x : list){
    chkRealloc(buff, sz = 1 + strlen(x) + strlen(buff));
    hRslt = StringCchCatA(buff, sz, x);
    if(FAILED(hRslt)){
      fprintf(stderr, "Error: catStr(): StringCchCatA() failed: ");
      if(hRslt==STRSAFE_E_INSUFFICIENT_BUFFER) fprintf(stderr, "INSUFFICIENT_BUFFER of %zu bytes\n", sz * sizeof(CHAR));
      else printErr(nullptr, sysErr, 23);
      clean_exit(16);
    }
  }
  return make_unique<LPSTR>(buff);
}

inline unique_ptr<LPWSTR> catWstr(initializer_list<LPCWSTR> list) {
  size_t sz = 0;
  HRESULT hRslt = S_OK;
  LPWSTR buff; chkAlloc(1, buff); buff[0] = '\0';
  for (auto x : list) {
    sz = 1 + (size_t)lstrlenW(x) + lstrlenW(buff);
    chkRealloc(buff, sz);
    hRslt = StringCchCatW(buff, sz, x);
    if (FAILED(hRslt)) {
      fprintf(stderr, "Error: catWstr(): StringCchCatW() failed: ");
      if (hRslt == STRSAFE_E_INSUFFICIENT_BUFFER) fprintf(stderr, "INSUFFICIENT_BUFFER of %zu bytes\n", sz * sizeof(WCHAR));
      else printErr(nullptr, sysErr, 24);
      clean_exit(16);
    }
  }
  return make_unique<LPWSTR>(buff);
}

inline void wstrCopy(LPWSTR dst, size_t sz, LPWSTR src, bool truncate){
  if(dst==nullptr) printErr("Error: wstrCopy(): destination cannot be null\n", 0, 15);
  auto hRslt = StringCchCopyW(dst, sz, src);
  if(hRslt==STRSAFE_E_INVALID_PARAMETER){
    flushErr("\n  Error: StringCchCopyW() failed: destination size cannot be larger than %ld\n", STRSAFE_MAX_CCH);
    printErr(nullptr, 0, 16);
  } else if(!truncate && hRslt==STRSAFE_E_INSUFFICIENT_BUFFER)
    printErr("Error: StringCchCopyW() failed: insufficient buffer\n", 0, 17);
}

inline bool getEnvVar(LPCWSTR var, LPWSTR &val){
	DWORD bufSz = 1025;
	LPWSTR buf; chkAlloc(bufSz, buf);

  DWORD ret = GetEnvironmentVariableW(var, buf, bufSz);
  if(0 == ret){
		DWORD err = GetLastError();
    if(ERROR_ENVVAR_NOT_FOUND == err){
			//flushOut("Environment variable does not exist.\n");
      return FALSE;
    }
  } 
	else if(bufSz < ret){
		chkRealloc(buf, ret);
		ret = GetEnvironmentVariable(var, buf, ret);
		if(!ret){
			printErr("GetEnvironmentVariable failed", sysErr, 0);
			return FALSE;
		}
	}

	val = *catWstr({buf});
  return TRUE;
}

string random_string(string::size_type length, bool digitsOnly){
  static auto &chrs = "0123456789"
    "abcdefghijklmnopqrstuvwxyz"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  static auto &digits = "0123456789";
  
  thread_local static mt19937 rg{ random_device{}() };
  thread_local static uniform_int_distribution<string::size_type> pick(0, (digitsOnly ? sizeof(digits) : sizeof(chrs)) - 2);
  
  string s;  s.reserve(length);
  while(length--){
    if(digitsOnly) s += digits[pick(rg)];
    else s += chrs[pick(rg)];
  }
  return s;
}

vector<string> split(string s, string delimiter){
  size_t pos_start = 0, pos_end, delim_len = delimiter.length();
  string token;
  vector<string> res;

  while((pos_end = s.find(delimiter, pos_start)) != string::npos){
    token = s.substr(pos_start, pos_end - pos_start);
    pos_start = pos_end + delim_len;
    res.push_back(token);
  }

  res.push_back(s.substr(pos_start));
  return res;
}

vector<string> split(const string &s, char delim){
  vector<string> result;
  stringstream ss(s);
  string item;

  while(getline(ss, item, delim)){
    result.push_back(item);
  }

  return result;
}
