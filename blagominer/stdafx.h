// stdafx.h: Includedatei für Standardsystem-Includedateien
// oder häufig verwendete projektspezifische Includedateien,
// die nur in unregelmäßigen Abständen geändert werden.
//

#pragma once
#include "common-pragmas.h"

#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <thread> 
#include <stdio.h>
#include <tchar.h>

// network.cpp, updatechecker.cpp
#include "ws2tcpip.h"
#include <windows.h> 
#include <urlmon.h>
#pragma comment(lib,"Ws2_32.lib")
#pragma comment(lib, "urlmon.lib")

#define RAPIDJSON_NO_SIZETYPEDEFINE

namespace rapidjson { typedef size_t SizeType; }
using namespace rapidjson;

#pragma warning( push )
#pragma warning( disable: 26495 )	// Warning C26451	"Variable 'rapidjson::.....' is uninitialized. Always initialize a member variable"
#pragma warning( disable: 26451 )	// Warning C26451	"Arithmetic overflow: Using operator '-' on a 4 byte value and then casting the result to a 8 byte value"
#pragma warning( disable: 26812 )	// Warning C26812	"The enum type 'rapidjson::.....' is unscoped. Prefer 'enum class' over 'enum'"
#pragma warning( disable: 4996 )	// Warning C4996	somehow _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING via compiler commandline didn't work?
#include "rapidjson/document.h"		// rapidjson's DOM-style API
#include "rapidjson/error/en.h"
#include "rapidjson/writer.h"
#pragma warning( pop )

#undef max

// TODO: Hier auf zusätzliche Header, die das Programm erfordert, verweisen.
