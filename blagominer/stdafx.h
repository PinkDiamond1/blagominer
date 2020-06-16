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
#pragma warning( disable: 26812 )	// Warning C26812	"The enum type 'rapidjson::.....' is unscoped. Prefer 'enum class' over 'enum'"
#include "rapidjson/document.h"		// rapidjson's DOM-style API
#include "rapidjson/error/en.h"
#include "rapidjson/writer.h"
#pragma warning( pop )


// TODO: Hier auf zusätzliche Header, die das Programm erfordert, verweisen.
