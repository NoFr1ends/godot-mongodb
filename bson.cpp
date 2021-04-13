#include "bson.h"

#include "core/io/marshalls.h"

#include "objectid.h"

#define MAKE_ROOM(m_amount) \
	if (data->size() < m_amount) data->resize(m_amount)

#define WRITE_TYPE(type) \
    MAKE_ROOM(position + 1); \
    data->write[position] = type; \
    position++;

#define WRITE_FIELD(name) \
    CharString cstr = name.utf8(); \
    auto field_name_length = encode_cstring(cstr.get_data(), nullptr); \
    MAKE_ROOM(position + field_name_length); \
    encode_cstring(cstr.get_data(), &data->write[position]); \
    position += field_name_length

int serialize_variant(String key, Variant value, Vector<uint8_t> *data, int position) {
    print_verbose("Serialize field " + key);

    switch(value.get_type()) {
        case Variant::REAL:
        {
            WRITE_TYPE(0x01);
            WRITE_FIELD(key);

            MAKE_ROOM(position + 8);
            encode_double(value, &data->write[position]);
            position += 8;
            break;
        }
        case Variant::STRING:
        {
            WRITE_TYPE(0x02);
            WRITE_FIELD(key);

            String value_str = value;
            CharString value_cstr = value_str.utf8();
            auto length = encode_cstring(value_cstr, nullptr);

            // write str length
            MAKE_ROOM(position + 4);
            encode_uint32(length, &data->write[position]);
            position += 4;
            MAKE_ROOM(position + length);
            encode_cstring(value_cstr, &data->write[position]);
            position += length;
            break;
        }
        case Variant::DICTIONARY:
        {
            WRITE_TYPE(0x03);
            WRITE_FIELD(key);

            position += Bson::serialize(value, data, position);
            break;
        }
        case Variant::ARRAY:
        {
            WRITE_TYPE(0x04);
            WRITE_FIELD(key);

            auto length_position = position;
            // reserve room for length
            MAKE_ROOM(position + 4);
            position += 4;

            Array arr = value;

            for(int i = 0; i < arr.size(); i++) {
                position = serialize_variant(itos(i), arr.get(i), data, position);
            }
            MAKE_ROOM(position + 1);
            data->write[position] = 0;
            position++;

            encode_uint32(position - length_position, &data->write[length_position]);
            break;
        }
        case Variant::POOL_BYTE_ARRAY:
        {
            WRITE_TYPE(0x05);
            WRITE_FIELD(key);

            PoolByteArray arr = value;

            MAKE_ROOM(position + 4);
            encode_uint32(arr.size(), &data->write[position]);
            position += 4;

            MAKE_ROOM(position + 1);
            data->write[position] = 0;
            position++;

            MAKE_ROOM(position + arr.size());
            auto reader = arr.read();
            copymem(&data->write[position], reader.ptr(), arr.size());
            position += arr.size();
            break;
        }
        // 0x06 undefined - deprecated
        // TODO:: 0x07 ObjectID
        case Variant::BOOL: 
        {
            WRITE_TYPE(0x08);
            WRITE_FIELD(key);

            MAKE_ROOM(position + 1);
            if((bool)value) {
                data->write[position] = 1;
            } else {
                data->write[position] = 0;
            }
            position++;
            break;
        }
        // TODO: 0x09 UTC datetime
        case Variant::NIL: 
        {
            WRITE_TYPE(0x0A);
            WRITE_FIELD(key);
            break;
        }
        // TODO: 0x0B regex
        // 0x0C DBPointer - deprecated
        // TODO: 0x0D javascript code
        // 0x0E symbol - deprecated
        // 0x0F javascript code w/ scope - deprecated
        // TODO: 0x10 32 bit integer - check if int is in bounds of 32 bit integer
        // TODO: 0x11 timestamp 
        case Variant::INT: 
        {
            WRITE_TYPE(0x12);
            WRITE_FIELD(key);
            MAKE_ROOM(position + 8);
            encode_uint64((uint64_t)value, &data->write[position]);
            position += 8;
            break;
        }
        // TODO: 0x13 128 bit decimal - we don't have that in gdscript
        default:
        {
            // TODO: Consider encode variant to a binary field with user defined subtype?
            ERR_BREAK_MSG(true, "Unsupported variant type to bson " + Variant::get_type_name(value.get_type()));
        }

    }
    return position;
}

int Bson::serialize(Dictionary dict, Vector<uint8_t> *data, int offset) {
    print_verbose("Serializing dictionary...");

    int position = offset;

    // make space for length
    MAKE_ROOM(position + 4);
    position += 4;

    // Loop over all keys and serialize them
    for(int i = 0; i < dict.size(); i++) {
        Variant key_variant = dict.get_key_at_index(i);
        if(key_variant.get_type() != Variant::STRING) {
            WARN_PRINT("Only string keys are allowed");
            continue;
        }

        String key = key_variant;
        Variant value = dict.get_value_at_index(i);
        
        position = serialize_variant(key, value, data, position);
    }

    // write null terminator
    MAKE_ROOM(position + 1);
    data->write[position] = 0;
    position += 1;

    // write length
    encode_uint32(position - offset, &data->write[offset]);

    return position - offset;
}

Variant deserialize_value(uint8_t type, Vector<uint8_t> *data, int *position) {
    Variant value;

    switch(type) {
        case 0x01: // double
        {
            value = decode_double(data->ptr() + *position);
            *position += 8;
            break;
        }
        case 0x02: // string
        {
            auto length = decode_uint32(data->ptr() + *position);
            *position += 4;
            CharString val;
            for(int i = 0; i < length; i++) {
                val += data->get((*position)++);
            }
            value = String(val);
            break;
        }
        case 0x03: // document
        {
            Dictionary dict;
            *position = Bson::deserialize(data, dict, *position);
            value = dict;
            break;
        }
        case 0x04: // array
        {
            print_verbose("deserializing array");
            auto length = decode_uint32(data->ptr() + *position);
            *position += 4;

            Array arr;
            auto arr_val_type = data->get(*position);
            (*position)++;
            while(arr_val_type != 0) {
                print_verbose("we have type: " + itos(arr_val_type) + " at " + itos(*position));
                // We can safely ignore the key here as the specs say that the array elements have to be in order
                uint8_t c;
                while(c = data->get(*position), c != 0x0) {
                    (*position)++;
                }
                (*position)++;

                Variant arr_val = deserialize_value(arr_val_type, data, position);
                arr.append(arr_val);

                arr_val_type = data->get(*position);
                (*position)++;
            }
            value = arr;

            break;
        }
        case 0x05: // binary 
        {
            auto length = decode_uint32(data->ptr() + *position);
            *position += 4 + length;
            break;
        }
        case 0x06: // undefined
        {
            break;
        }
        case 0x07: // object id
        {
            Ref<MongoObjectID> objectid = memnew(MongoObjectID());

            copymem(objectid->get_data(), data->ptr() + *position, 12);
            *position += 12;

            value = objectid;

            break;
        }
        case 0x08: // boolean
        {
            value = data->get(*position) > 0;
            *position += 1;
            break;
        }
        case 0x09: // utc datetime
        {
            *position += 8;
            break;
        }
        case 0x0A: // null value 
        {
            break;
        }
        case 0x0B: // regex
        {
            break;
        }
        case 0x0C: // dbpointer
        {
            break;
        }
        case 0x0D: // javascript code
        {
            break;
        }
        case 0x0E: // symbol
        {
            break;
        }
        case 0x0F: // javascript code w/ scope
        { 
            break;
        }
        case 0x10: // 32 bit int 
        {
            value = (int32_t)decode_uint32(data->ptr() + *position);
            *position += 4;
            break;
        }
        case 0x11: // utc timestamp
        {
            *position += 8;
            break;
        }
        case 0x12: // 64 bit int 
        {
            value = (int64_t)decode_uint64(data->ptr() + *position);
            *position += 8;
            break;
        }
        case 0x13: // 128 bit decimal
        {
            *position += 16;
            break;
        }
        default: {
            ERR_FAIL_V_MSG(0, "Unknown bson type " + itos(type));
            break;
        }
    }

    return value;
}

int Bson::deserialize(Vector<uint8_t> *data, Dictionary &dict, int offset) {
    int position = offset;

    auto length = decode_uint32(data->ptr() + position);
    position += 4;

    uint8_t type = data->get(position);
    position += 1;
    while(type != 0) {
        print_verbose("we're at " + itos(position));

        String key;
        uint8_t c;
        while(c = data->get(position), c != 0x0) {
            key += c;

            position++;
        }
        position++;

        print_verbose(key);

        auto value = deserialize_value(type, data, &position);

        dict[key] = value;
        print_verbose(value);

        type = data->get(position);
        position += 1;
    } 
    
    // We should have reached the end
    return position;
}