#include "objectid.h"

MongoObjectID::MongoObjectID(PoolByteArray &data) : m_data(std::move(data)) {

}

String MongoObjectID::to_string() {
    PoolByteArray::Read r = m_data.read();
    return "ObjectId(\"" + String::hex_encode_buffer(&r[0], m_data.size()) + "\")";
}

void MongoObjectID::_bind_methods() {

}