// Copyright 2013 Daniel Parker
// Distributed under the Boost license, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See https://sourceforge.net/projects/jsoncons/files/ for latest version
// See https://sourceforge.net/p/jsoncons/wiki/Home/ for documentation.

#ifndef JSONCONS_EXT_CBOR_CBOR_SERIALIZER_HPP
#define JSONCONS_EXT_CBOR_CBOR_SERIALIZER_HPP

#include <string>
#include <sstream>
#include <vector>
#include <ostream>
#include <cstdlib>
#include <map>
#include <cmath>
#include "jsoncons/jsoncons.hpp"
#include "jsoncons/output_format.hpp"
#include "jsoncons/json2.hpp"
#include "jsoncons/json_output_handler.hpp"
#include <limits> // std::numeric_limits

namespace jsoncons_ext { namespace cbor {

template<typename Char,class Alloc>
class basic_cbor_serializer : public jsoncons::basic_json_output_handler<Char>
{
public:
    basic_cbor_serializer(std::basic_ostream<Char>& os)
       :
       os_(std::addressof(os))
    {
    }

    ~basic_cbor_serializer()
    {
    }

private:
    void put_major( uint8_t major, uint8_t additional ) {
      os_->put(major << 5 | additional);
    }

    void put_additional( uint8_t major, uint64_t additional ) {
      if( additional < 24ull ) {
        put_major( major, additional );
      } else if( additional < 256ull ) {
        put_major( major, 24 );
        os_->put( ( additional ) & 0xff );
      } else if( additional < 65536ull ) {
        put_major( major, 25 );
        os_->put( ( additional >> 8 ) & 0xff );
        os_->put( ( additional ) & 0xff );
      } else if( additional < 4294967296ull ) {
        put_major( major, 26 );
        os_->put( ( additional >> 24 ) & 0xff );
        os_->put( ( additional >> 16 ) & 0xff );
        os_->put( ( additional >> 8 ) & 0xff );
        os_->put( ( additional ) & 0xff );
      } else {
        put_major( major, 27 );
        os_->put( ( additional >> 56 ) & 0xff );
        os_->put( ( additional >> 48 ) & 0xff );
        os_->put( ( additional >> 40 ) & 0xff );
        os_->put( ( additional >> 32 ) & 0xff );
        os_->put( ( additional >> 24 ) & 0xff );
        os_->put( ( additional >> 16 ) & 0xff );
        os_->put( ( additional >> 8 ) & 0xff );
        os_->put( ( additional ) & 0xff );
      }
    }

    virtual void do_begin_json()
    {
    }

    virtual void do_end_json()
    {
    }

    virtual void do_begin_object()
    {
      put_major( 5, 31 );
    }

    virtual void do_end_object()
    {
      put_major( 7, 31 );
    }

    virtual void do_begin_array()
    {
      put_major( 4, 31 );
    }

    virtual void do_end_array()
    {
      put_major( 7, 31 );
    }

    virtual void do_name(const Char* name, size_t length)
    {
      put_additional( 3, length );
      os_->write( name, length );
    }

    virtual void do_null_value()
    {
      put_additional( 7, 22 );
    }

    virtual void do_string_value(const Char* val, size_t length)
    {
      put_additional( 3, length );
      os_->write( val, length );
    }

    virtual void do_binary_value(const Char* val, size_t length)
    {
      put_additional( 2, length );
      os_->write( val, length );
    }

    virtual void do_double_value(double val)
    {
      int exponent;
      double significand = frexp( val, &exponent );
      int64_t fraction = static_cast< int64_t >( frexp( val, &exponent ) * 9007199254740992 ) & 0x000fffffffffffffull;
      
      if( exponent < 127 && exponent > -126 && ( fraction & 0x000000001fffffffull ) == 0 ) {
        fraction >>= 29;
        uint32_t bits = val < 0 ? 0x80000000ul : 0ul;
        bits |= ( static_cast< uint32_t >( exponent + 126 ) << 23 ) & 0x7f100000ul;
        bits |= fraction;

        put_major( 7, 26 );
        os_->put( ( bits >> 24 ) & 0xff );
        os_->put( ( bits >> 16 ) & 0xff );
        os_->put( ( bits >> 8 ) & 0xff );
        os_->put( ( bits ) & 0xff );
      } else {
        uint64_t bits = val < 0 ? 0x8000000000000000ull : 0ull;
        bits |= ( static_cast< int64_t >( exponent + 1022 ) << 52 ) & 0x7ff0000000000000ull;
        bits |= fraction;

        put_major( 7, 27 );
        os_->put( ( bits >> 56 ) & 0xff );
        os_->put( ( bits >> 48 ) & 0xff );
        os_->put( ( bits >> 40 ) & 0xff );
        os_->put( ( bits >> 32 ) & 0xff );
        os_->put( ( bits >> 24 ) & 0xff );
        os_->put( ( bits >> 16 ) & 0xff );
        os_->put( ( bits >> 8 ) & 0xff );
        os_->put( ( bits ) & 0xff );
      }
    }

    virtual void do_longlong_value(long long val)
    {
      if( val < 0 ) {
        put_additional( 1, - 1 - val );
      } else {
        put_additional( 0, val );
      }
    }

    virtual void do_ulonglong_value(unsigned long long val)
    {
      put_additional( 0, val );
    }

    virtual void do_bool_value(bool val)
    {
      if( val ) {
        put_additional( 7, 21 );
      } else {
        put_additional( 7, 22 );
      }
    }

    std::basic_ostream<Char>* os_;
};

typedef basic_cbor_serializer<char,std::allocator<void>> cbor_serializer;

}}

#endif
