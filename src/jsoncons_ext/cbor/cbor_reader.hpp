#ifndef JSONCONS_EXT_CBOR_CBOR_READER_HPP
#define JSONCONS_EXT_CBOR_CBOR_READER_HPP

#include <string>
#include <sstream>
#include <vector>
#include <istream>
#include <cstdlib>
#include <stdexcept>
#include <cmath>
#include "jsoncons/jsoncons.hpp"
#include "jsoncons/json_input_handler.hpp"
#include "jsoncons/parse_error_handler.hpp"
#include "jsoncons/json.hpp"

namespace jsoncons_ext { namespace cbor {

template<typename Char,class Alloc>
class basic_cbor_reader : private jsoncons::basic_parsing_context<Char>
{
  public:
    basic_cbor_reader(std::basic_istream<Char>& is,
                      jsoncons::basic_json_input_handler<Char>& handler)
      : is_(std::addressof(is)), handler_(std::addressof(handler))
    {
      //init(jsoncons::json::an_object);
    }
    
    void read() {
      parse_thing();
    }
    
  private:
    std::basic_istream<Char>* is_;
    jsoncons::basic_json_input_handler<Char>* handler_;
    
    virtual unsigned long do_line_number() const {
      return 1;
    }

    virtual unsigned long do_column_number() const {
      return 0;
    }

    virtual bool do_eof() const {
      return false;
    }

    virtual size_t do_minimum_structure_capacity() const {
      return 0;
    }

    virtual char do_last_char() const {
      return '\0';
    }
    
    uint64_t parse_additional( uint8_t additional ) {
      if( additional < 24 ) {
        return additional;
      } else if( additional < 28 ) {
        int bytes = 1 << ( additional - 24 );
        uint64_t ret = 0;
        for( ; bytes > 0; --bytes ) {
          ret <<= 8;
          Char byte;
          is_->get( byte );
          ret |= ( byte & 0xff );
        }
        return ret;
      } else {
        JSONCONS_THROW_EXCEPTION("Invalid additional information.");
      }
    }
    
    std::tuple< uint8_t, uint8_t > parse_tag() {
      Char tag;
      is_->get( tag );
      
      uint8_t major = ( tag & 0xe0 ) >> 5;
      uint8_t additional = tag & 0x1f;
      
      return std::make_tuple( major, additional );
    }
    
    std::vector< Char > parse_string( uint8_t submajor, uint8_t additional );
    
    void parse_integer_pos( uint8_t additional ) {
      handler_->value( parse_additional( additional ), *this );
    }
    
    void parse_integer_neg( uint8_t additional ) {
      handler_->value( - 1 - parse_additional( additional ), *this );
    }
    
    void parse_binary( uint8_t additional );
    
    void parse_text( uint8_t additional );
    
    void parse_array( uint8_t additional );
    
    void parse_map( uint8_t additional );
    
    void parse_semantic( uint8_t additional );
    
    void parse_primitive( uint8_t additional );
    
    void parse_thing() {
      uint8_t major;
      uint8_t additional;
      
      std::tie( major, additional ) = parse_tag();
      

      switch( major ) {
        case 0:
          parse_integer_pos( additional );
          break;
        case 1:
          parse_integer_neg( additional );
          break;
        case 2:
          parse_binary( additional );
          break;
        case 3:
          parse_text( additional );
          break;
        case 4:
          parse_array( additional );
          break;
        case 5:
          parse_map( additional );
          break;
        case 6:
          parse_semantic( additional );
          break;
        case 7:
          parse_primitive( additional );
          break;
        default:
          JSONCONS_THROW_EXCEPTION("Invalid major type.");
      }
    }

};

template<typename Char,class Alloc>
std::vector< Char > basic_cbor_reader< Char, Alloc >::parse_string( uint8_t submajor, uint8_t additional ) {
  ssize_t length = -1;
  
  if( additional != 31 ) {
    length = parse_additional( additional );
  }
  
  if( length < 0 ) {
    std::vector< Char > ret;
    auto iter = std::back_inserter( ret );
    
    while( true ) {
      uint8_t major;
      uint8_t additional;
      std::tie( major, additional ) = parse_tag();
      
      if( major == 7 && additional == 31 ) {
        break;
      }
      
      if( major != submajor ) {
        JSONCONS_THROW_EXCEPTION("Invalid major for indefinite length string.");
      }
      
      auto tmp = parse_string( submajor, additional );
      std::copy( tmp.begin(), tmp.end(), iter );
    }
    
    return ret;
    
    JSONCONS_THROW_EXCEPTION("Indefinite string encoding is not yet supported.");
  } else {
    std::vector< Char > buffer( length );
    is_->read( buffer.data(), length );
    return buffer;
  }
}

template<typename Char,class Alloc>
void basic_cbor_reader< Char, Alloc >::parse_binary( uint8_t additional ) {
  auto data = parse_string( 2, additional );
  handler_->value( data, *this );
}

template<typename Char,class Alloc>
void basic_cbor_reader< Char, Alloc >::parse_text( uint8_t additional ) {
  auto string = parse_string( 3, additional );
  handler_->value( string.data(), string.size(), *this );
}

template<typename Char,class Alloc>
void basic_cbor_reader< Char, Alloc >::parse_array( uint8_t additional ) {
  ssize_t length = -1;
  
  if( additional != 31 ) {
    length = parse_additional( additional );
  }
  
  handler_->begin_array( *this );

  while( true ) {
    if( length == 0 ) {
      break;
    } else if( length < 0 ) {
      int peek = is_->peek();
      if( peek == 0xff ) {
        is_->get();
        break;
      }
    }
    
    parse_thing();
  }
  
  handler_->end_array( *this );
}
    
template<typename Char,class Alloc>
void basic_cbor_reader< Char, Alloc >::parse_map( uint8_t additional ) {
  ssize_t length = -1;
  
  if( additional != 31 ) {
    length = parse_additional( additional );
  }
  
  handler_->begin_object( *this );

  while( true ) {
    if( length == 0 ) {
      break;
    } else if( length < 0 ) {
      int peek = is_->peek();
      if( peek == 0xff ) {
        is_->get();
        break;
      }
    }
    
    uint8_t major;
    uint8_t additional;
    std::tie( major, additional ) = parse_tag();
    
    if( major != 3 ) {
      JSONCONS_THROW_EXCEPTION("Invalid major for map name.");
    }
    auto name = parse_string( 3, additional );
    
    handler_->name( name.data(), name.size(), *this );
    
    parse_thing();
  }
  
  handler_->end_object( *this );
}
    
template<typename Char,class Alloc>
void basic_cbor_reader< Char, Alloc >::parse_semantic( uint8_t additional ) {
  auto tag = parse_additional( additional );
  parse_thing();
}
    
template<typename Char,class Alloc>
void basic_cbor_reader< Char, Alloc >::parse_primitive( uint8_t additional ) {
  if( additional < 25 ) {
    additional = parse_additional( additional );
    switch( additional ) {
      case 20:
        handler_->value( false, *this );
        break;
      case 21:
        handler_->value( true, *this );
        break;
      case 22:
        handler_->value( false, *this );
        break;
      case 23:
        JSONCONS_THROW_EXCEPTION("Undefined is unrepresentable in jsoncons.");
        break;
      default:
        JSONCONS_THROW_EXCEPTION("Unknown simple type.");
        break;
    }
  } else if( additional < 28 ) {
    uint16_t sign = 0;
    uint64_t fraction = 0;
    int32_t exponent = 0;
    switch( additional ) {
      case 26: {
        uint64_t data = parse_additional( additional );
        sign = ( data & 0x80000000 ) ? -1 : 1;
        exponent = ( ( data & 0x7f100000 ) >> 23 ) - 127 - 23;
        fraction = ( data & 0x007fffff ) + 0x00800000;
        break;
      }
      default:
        JSONCONS_THROW_EXCEPTION("Unknown floating point size.");
    }
    handler_->value( sign * ldexp( fraction, exponent ), *this );
  } else if( additional == 31 ) {
    JSONCONS_THROW_EXCEPTION("Unexpected stop code.");
  } else {
    JSONCONS_THROW_EXCEPTION("Unknown primitive type.");
  }
}
    

typedef basic_cbor_reader<char,std::allocator<void>> cbor_reader;
  
}}

#endif
