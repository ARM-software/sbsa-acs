/** @file
 * Copyright (c) 2016, ARM Limited or its affiliates. All rights reserved.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

///
/// Common header region in PCI Configuration Space
/// Section 6.1, PCI Local Bus Specification, 2.2
///
typedef struct {
  uint16_t  vendor_id;
  uint16_t  device_id;
  uint16_t  command;
  uint16_t  status;
  uint8_t   revision_id;
  uint8_t   class_code[3];
  uint8_t   cache_line;
  uint8_t   latency_timer;
  uint8_t   header_type;
  uint8_t   BIST;
} PCI_DEVICE_INDEPENDENT_REGION;

///
/// PCI Device header region in PCI Configuration Space
/// Section 6.1, PCI Local Bus Specification, 2.2
///
typedef struct {
  uint32_t  Bar[6];
  uint32_t  CISPtr;
  uint16_t  SubsystemVendorID;
  uint16_t  SubsystemID;
  uint32_t  ExpansionRomBar;
  uint8_t   CapabilityPtr;
  uint8_t   Reserved1[3];
  uint32_t  Reserved2;
  uint8_t   InterruptLine;
  uint8_t   InterruptPin;
  uint8_t   MinGnt;
  uint8_t   MaxLat;
} PCI_DEVICE_HEADER_TYPE_REGION;

///
/// PCI Device Configuration Space
/// Section 6.1, PCI Local Bus Specification, 2.2
///
typedef struct {
  PCI_DEVICE_INDEPENDENT_REGION Hdr;
  PCI_DEVICE_HEADER_TYPE_REGION Device;
} PCI_TYPE00;

///
/// PCI-PCI Bridge header region in PCI Configuration Space
/// Section 3.2, PCI-PCI Bridge Architecture, Version 1.2
///
typedef struct {
  uint32_t  Bar[2];
  uint8_t   PrimaryBus;
  uint8_t   SecondaryBus;
  uint8_t   SubordinateBus;
  uint8_t   SecondaryLatencyTimer;
  uint8_t   IoBase;
  uint8_t   IoLimit;
  uint16_t  SecondaryStatus;
  uint16_t  MemoryBase;
  uint16_t  MemoryLimit;
  uint16_t  PrefetchableMemoryBase;
  uint16_t  PrefetchableMemoryLimit;
  uint32_t  PrefetchableBaseUpper32;
  uint32_t  PrefetchableLimitUpper32;
  uint16_t  IoBaseUpper16;
  uint16_t  IoLimitUpper16;
  uint8_t   CapabilityPtr;
  uint8_t   Reserved[3];
  uint32_t  ExpansionRomBAR;
  uint8_t   InterruptLine;
  uint8_t   InterruptPin;
  uint16_t  BridgeControl;
} PCI_BRIDGE_CONTROL_REGISTER;

///
/// PCI-to-PCI Bridge Configuration Space
/// Section 3.2, PCI-PCI Bridge Architecture, Version 1.2
///
typedef struct {
  PCI_DEVICE_INDEPENDENT_REGION Hdr;
  PCI_BRIDGE_CONTROL_REGISTER   Bridge;
} PCI_TYPE01;

