#include "objectid.h"

#include "core/os/os.h"
#include "core/io/marshalls.h"

uint32_t MongoObjectID::m_counter = 0;
RandomPCG MongoObjectID::m_random;

void MongoObjectID::initialize() {
    m_random.randomize();
    m_counter = m_random.rand();
}

MongoObjectID::MongoObjectID() {
    m_data = new uint8_t[12] { };

    // NOTE: timestamp is 4 byte by mongodb definition
    auto timestamp = OS::get_singleton()->get_unix_time();
    uint64_t random = m_random.rand() << 32 | m_random.rand();
    auto counter = m_counter++;
    // timestamp and counter are big endian
    timestamp = BSWAP32(timestamp);
    counter = BSWAP32(counter) >> 8; // counter is only 3 byte long

    encode_uint32(timestamp, &m_data[0]);
    for (int i = 0; i < 5; i++) {
		m_data[4+i] = random & 0xFF;
		random >>= 8;
	}
    for (int i = 0; i < 3; i++) {
		m_data[4+5+i] = counter & 0xFF;
		counter >>= 8;
	}
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