#include "objectid.h"

MongoObjectID::MongoObjectID() {
    m_data = new uint8_t[12] { };
}

MongoObjectID::~MongoObjectID() {
    delete[] m_data;
}

String MongoObjectID::str() {
    return String::hex_encode_buffer(&m_data[0], 12);
}

int c2i(CharType i) {
    if(i >= '0' && i <= '9') {
        return i - '0';
    }
    if(i >= 'a' && i <= 'f') {
        return i - 'a' + 10;
    }
    return 0;
}

void MongoObjectID::from_string(String str) {
    ERR_FAIL_COND_MSG(str.length() != 24, "MongoDB Object IDs need to be 12 bytes long");
    str = str.to_lower();
    for(int i = 0; i < str.length(); i+=2) {
        m_data[i/2] = (c2i(str.get(i)) * 16) + c2i(str.get(i+1));
    }
}

String MongoObjectID::to_string() {
    return str();
}

void MongoObjectID::_bind_methods() {
    ClassDB::bind_method(D_METHOD("from_string", "id"), &MongoObjectID::from_string);
}