#include "objectid.h"

MongoObjectID::MongoObjectID() {
    m_data = new uint8_t[12] { };
}

MongoObjectID::~MongoObjectID() {
    delete[] m_data;
}

String MongoObjectID::to_string() {
    return "ObjectId(\"" + String::hex_encode_buffer(&m_data[0], 12) + "\")";
}

void MongoObjectID::_bind_methods() {

}