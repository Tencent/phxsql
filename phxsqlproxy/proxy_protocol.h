/*
 Copyright (c) 2016 Tencent.  See the AUTHORS file for names
 of contributors.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public
 License along with this library; if not, write to the
 Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 Boston, MA  02110-1301, USA.

 */

#pragma once

namespace phxsqlproxy {

typedef struct tagProxyHdrV1 {
    char line[108];
} ProxyHdrV1_t;

typedef struct tagProxyHdrV2 {
    uint8_t sig[12];
    uint8_t ver_cmd;
    uint8_t fam;
    uint16_t len;
    union {
        struct {  // for TCP/UDP over IPv4, len = 12
            uint32_t src_addr;
            uint32_t dst_addr;
            uint16_t src_port;
            uint16_t dst_port;
        } __attribute__((packed)) ip4;
        struct {  // for TCP/UDP over IPv6, len = 36
            uint8_t  src_addr[16];
            uint8_t  dst_addr[16];
            uint16_t src_port;
            uint16_t dst_port;
        } __attribute__((packed)) ip6;
    } addr;
} __attribute__((packed)) ProxyHdrV2_t;

const uint32_t PP2_VERSION = 0x20;
const uint32_t PP2_CMD_LOCAL = 0x00;
const uint32_t PP2_CMD_PROXY = 0x01;
const uint32_t PP2_FAM_UNSPEC = 0x00;
const uint32_t PP2_FAM_TCP4 = 0x11;
const uint32_t PP2_FAM_TCP6 = 0x21;

const char * const PP2_SIGNATURE = "\x0D\x0A\x0D\x0A\x00\x0D\x0A\x51\x55\x49\x54\x0A";

}
