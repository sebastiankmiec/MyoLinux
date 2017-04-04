/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BLED112CLIENT_H
#define BLED112CLIENT_H

#include "bleapi.h"
#include "firstargument.h"
#include "serial.h"

#include <functional>
#include <map>

class Bled112Client {
public:
    template <typename Function>
    using DisableIfFirstArgumentIsPartial = std::enable_if<!Partial<typename FirstArgument<Function>::type>::value>;

    template <typename Function>
    using EnableIfFirstArgumentIsPartial = std::enable_if<Partial<typename FirstArgument<Function>::type>::value>;

    Bled112Client(const Serial &socket)
        : socket(socket)
    { }

    template <typename T>
    void write(const T &);

    template <typename T>
    void write(const T &, const Buffer &);

    template <typename T>
    T read();

    template <typename T>
    T read(Buffer &);

    template <typename... Functions>
    void read(const Functions&...);

private:
    Header readHeader();

    template <typename T>
    void checkHeader(const Header &);

    template <typename T>
    T readPayload(const Header &);

    template <typename T>
    T readPayload(const Header &, Buffer &);

    void dispatch(const Header &);

    template <typename Function, typename... Functions>
    auto dispatch(const Header &, const Function &, const Functions&...)
        ->  typename DisableIfFirstArgumentIsPartial<Function>::type;

    template <typename Function, typename... Functions>
    auto dispatch(const Header &, const Function &, const Functions&...)
        -> typename EnableIfFirstArgumentIsPartial<Function>::type;

    Serial socket;
};

template <typename T>
void Bled112Client::write(const T &payload)
{
    socket.write(pack(getHeader<T>()));
    socket.write(pack(payload));
}

template <typename T>
void Bled112Client::write(const T &payload, const Buffer &leftover)
{
    socket.write(pack(getHeader<T>(leftover.size())));
    socket.write(pack(payload));
    socket.write(leftover);
}

inline Header Bled112Client::readHeader()
{
    return unpack<Header>(socket.read(sizeof(Header)));
}

template <typename T>
void Bled112Client::checkHeader(const Header &header)
{
    if (header.cls != T::cls) {
        throw std::runtime_error("Class index does not match the expected value.");
    }
    if (header.cmd != T::cmd) {
        throw std::runtime_error("Command index does not match the expected value.");
    }
    if (!Partial<T>::value && header.length() != sizeof(T)) {
        throw std::runtime_error("Payload size does not match the expected value.");
    }
}

template <typename T>
T Bled112Client::readPayload(const Header &header)
{
    return unpack<T>(socket.read(header.length()));
}

template <typename T>
T Bled112Client::readPayload(const Header &header, Buffer &leftover)
{
    const auto payload = unpack<T>(socket.read(sizeof(T)));
    leftover = socket.read(header.length() - sizeof(T));
    return payload;
}

template <typename T>
T Bled112Client::read()
{
    const auto header = readHeader();
    checkHeader<T>(header);
    return readPayload<T>(header);
}

template <typename T>
T Bled112Client::read(Buffer &leftover)
{
    const auto header = readHeader();
    checkHeader<T>(header);
    return readPayload<T>(header, leftover);
}

inline void Bled112Client::dispatch(const Header &) { }

template <typename Function, typename... Functions>
auto Bled112Client::dispatch(const Header &header, const Function &function, const Functions&... functions)
    -> typename DisableIfFirstArgumentIsPartial<Function>::type
{
    using T = typename FirstArgument<Function>::type;

    if (header.cls == T::cls && header.cmd == T::cmd && header.length() == sizeof(T)) {
        function(readPayload<T>(header));
        return;
    }
    dispatch(header, functions...);
}

template <typename Function, typename... Functions>
auto Bled112Client::dispatch(const Header &header, const Function &function, const Functions&... functions)
    -> typename EnableIfFirstArgumentIsPartial<Function>::type
{
    using T = typename FirstArgument<Function>::type;

    if (header.cls == T::cls && header.cmd == T::cmd) {
        Buffer leftover;
        const auto payload = readPayload<T>(header, leftover);

        function(std::move(payload), std::move(leftover));
        return;
    }
    dispatch(header, functions...);
}

// The dispatch works by iterating over a list of functions, the right one is selected based on the first argument.
// In the case that the data type is partial an additional argument is required to pass the payload.
//
// Accepted function signatures:
// void(Type)
// void(Type, Buffer)

template <typename... Functions>
void Bled112Client::read(const Functions&... functions)
{
    const auto header = readHeader();
    dispatch(header, functions...);
}

#endif // BLED112CLIENT_H
