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
#include <random>
using namespace std;
// ==========================================


// ==========================================
constexpr int MAX_LOADSTRING = 100;

constexpr int g_iViewsizeX = 1200;
constexpr int g_iViewsizeY = 700;

constexpr int g_iMonsterCnt = 10;
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

	END
};

enum class KEY
{
	W,
	A,
	S,
	D,

	Q,
	E,
	T,
	Z,
	X,
	C,
	V,

	UP,
	DOWN,
	LEFT,
	RIGHT,

	SPACE,
	ENTER,
	CTRL,
	LSHIFT,
	RSHIFT,
	ESC,

	LBUTTON,
	RBUTTON,

	END,
};

const int KeyMap[(int)KEY::END]
{
	'W',
	'A',
	'S',
	'D',

	'Q',
	'E',
	'T',
	'Z',
	'X',
	'C',
	'V',

	VK_UP,
	VK_DOWN,
	VK_LEFT,
	VK_RIGHT,

	VK_SPACE,
	VK_RETURN,
	VK_CONTROL,
	VK_LSHIFT,
	VK_RSHIFT,
	VK_ESCAPE,

	VK_LBUTTON,
	VK_RBUTTON,
};

enum class KEY_STATE
{
	NONE,
	TAP,
	HOLD,
	AWAY,
};

struct KeyInfo
{
	KEY_STATE eState;
	bool bPrevPush;
};


#pragma comment(lib, "msimg32.lib")
