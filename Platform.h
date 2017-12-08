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
#include <iostream>
#include <tchar.h>
#include <fcntl.h>

#include <safeint.h>
using msl::utilities::SafeInt;

#include <mstcpip.h>
#include <Ws2tcpip.h>
#pragma comment(lib, 	"Ws2_32.lib")