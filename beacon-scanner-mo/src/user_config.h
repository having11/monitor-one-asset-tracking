/*
 * Copyright (c) 2023 Particle Industries, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "tracker_config.h"             // For TSOM related configuration
#include "monitor_one_config.h"         // For Monitor One related configuration

#define MONITOR_ONE_SUPPORT_PROTO       (1)
#define MONITOR_ONE_SUPPORT_IOEXP       (1)

using MonitorOneCardFunction = std::function<int(void)>;

#if defined(MONITOR_ONE_SUPPORT_PROTO) && MONITOR_ONE_SUPPORT_PROTO
constexpr char MONITOREDGE_PROTO_SKU[]  {"EXP1_PROTO"};
#endif // MONITOR_ONE_SUPPORT_PROTO


#if defined(MONITOR_ONE_SUPPORT_IOEXP) && MONITOR_ONE_SUPPORT_IOEXP
#include "monitor_edge_ioexpansion.h"
int expanderIoInit();
int expanderIoLoop();
#endif // MONITOR_ONE_SUPPORT_IOEXP
