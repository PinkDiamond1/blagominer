// stdafx.h: Includedatei für Standardsystem-Includedateien
// oder häufig verwendete projektspezifische Includedateien,
// die nur in unregelmäßigen Abständen geändert werden.
//

#pragma once

#include "targetver.h"
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>
#include <map>
#include <thread> 
#include "ws2tcpip.h"
#include <windows.h> 
#include <stdio.h>
#include <tchar.h>

#define RAPIDJSON_NO_SIZETYPEDEFINE

namespace rapidjson { typedef size_t SizeType; }
using namespace rapidjson;

#include "rapidjson/document.h"		// rapidjson's DOM-style API
#include "rapidjson/error/en.h"
#include "rapidjson/writer.h"


// TODO: Hier auf zusätzliche Header, die das Programm erfordert, verweisen.
