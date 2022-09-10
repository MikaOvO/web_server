#pragma once

#include <string.h>
#include <time.h>
#include <functional>
#include <queue>
#include <vector>
#include <map>
#include <thread>
#include <cassert>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

void AddSig(int sig, void(handler)(int), bool restart);
