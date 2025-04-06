#pragma once
#include "targetver.h"
#include "resource.h"

#define WIN32_LEAN_AND_MEAN   
#include <windows.h>
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// ==========================================
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <list>
#include <cassert>
using namespace std;
// ==========================================


// ==========================================
constexpr int MAX_LOADSTRING = 100;

constexpr int g_iViewsizeX = 1200;
constexpr int g_iViewsizeY = 700;
// ==========================================


struct vPoint
{
	float x;
	float y;
};

enum class DIR_TYPE
{
	LEFT,
	RIGHT,
	UP,
	DOWN,

	COUNT
};

#pragma comment(lib, "msimg32.lib")
