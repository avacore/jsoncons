// Copyright 2013 Daniel Parker
// Distributed under Boost license

#ifndef JSONCONS_BASIC_JSON_HANDLER_HPP
#define JSONCONS_BASIC_JSON_HANDLER_HPP

#include <string>

namespace jsoncons {

template <class Char>
class basic_json_handler
{
public:
    virtual void begin_json() = 0;

    virtual void end_json() = 0;

    virtual void begin_object() = 0;

    virtual void end_object() = 0;

    virtual void begin_array() = 0;

    virtual void end_array() = 0;

    virtual void name(const std::basic_string<Char>& name) = 0;

    virtual void value(const std::basic_string<Char>& value) = 0;

    virtual void value(double value) = 0;

    virtual void value(long long value) = 0;

    virtual void value(unsigned long long value) = 0;

    virtual void value(bool value) = 0;

    virtual void null() = 0;
};

typedef basic_json_serializer<char> json_serializer;

}

#endif