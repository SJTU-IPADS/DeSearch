#include "json.hpp"

namespace json {

JSON Array() {
    return std::move( JSON::Make( JSON::Class::Array ) );
}

template <typename... T>
JSON Array( T... args ) {
    JSON arr = JSON::Make( JSON::Class::Array );
    arr.append( args... );
    return std::move( arr );
}

JSON Object() {
    return std::move( JSON::Make( JSON::Class::Object ) );
}

std::ostream& operator<<( std::ostream &os, const JSON &json ) {
    os << json.dump();
    return os;
}

JSON JSON::Load( const string &str ) {
    size_t offset = 0;
    return std::move( parse_next( str, offset ) );
}

} // End Namespace json