// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <SDKDDKVer.h>
#include <windows.h>
#include <intrin.h>
#include <cstdint>
#include <WinSock2.h>
#include <Mswsock.h>
#include <vector>
#include <array>

#include <stdio.h>
#include <tchar.h>
#include <fcntl.h>


// TODO: reference additional headers your program requires here
#include <safeint.h>
#include <mstcpip.h>
using msl::utilities::SafeInt;
#include "ClsSockets.h"
#include <Ws2tcpip.h>
#pragma comment(lib, 	"Ws2_32.lib")
