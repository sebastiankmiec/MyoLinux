/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once
#ifndef GATTCLIENT_H
#define GATTCLIENT_H

#include "buffer.h"
#include "bled112client.h"

#include <cstdint>

#include <map>
#include <array>

class GattClient {
public:
    using Address = std::array<std::uint8_t, 6>;
    using Characteristics = std::map<Buffer, std::uint16_t>;

    GattClient(const Bled112Client &);

    void discover();
    Characteristics characteristics();
    void connect(const Address &);
    void disconnect();
    void writeAttribute(const std::uint16_t, const Buffer &);
    Buffer readAttribute(const std::uint16_t);
    void readAttribute(const std::function<void(std::uint16_t handle, const Buffer &)> &callback);

private:
    Bled112Client client;
    std::uint8_t connection;
};

#endif // GATTCLIENT_H
