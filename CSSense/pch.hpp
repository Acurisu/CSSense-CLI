#pragma once

#include <algorithm>
#include <iostream>
#include <chrono>

#include <tclap/CmdLine.h>

#include <winreg/WinReg.hpp>

#include <cpprest/asyncrt_utils.h>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <cpprest/uri.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <agents.h>

#include "BluetoothLEController/BluetoothLEController/BluetoothLEController.hpp"
#include "Gist/Color.h"