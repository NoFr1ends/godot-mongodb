#include "bson_helper.h"

#include <bsoncxx/builder/basic/array.hpp>

using bsoncxx::builder::basic::kvp;

bsoncxx::document::value convert_dictionary_to_document(Dictionary dict) {
    auto builder = bsoncxx::builder::basic::document{};

    for(int i = 0; i < dict.size(); i++) {
        auto key = dict.get_key_at_index(i);
        auto value = dict.get_value_at_index(i);

        if(key.get_type() != Variant::STRING) {
            WARN_PRINT("Only string keys are allowed.");
            continue;
        }

        auto utf8_key = ((String)key).utf8();
        std::string key_str = std::string(utf8_key.get_data());

        switch(value.get_type()) {
            case Variant::NIL:
                //builder.append(kvp(key_str, nullptr));
                break;
            case Variant::BOOL:
                builder.append(kvp(key_str, (bool)value));
                break;
            case Variant::INT:
                builder.append(kvp(key_str, (int)value));
                break;
            case Variant::REAL:
                builder.append(kvp(key_str, (float)value));
                break;
            case Variant::STRING:
            {
                String gd_str = value;
                CharString char_data = gd_str.utf8();
                std::string cpp_str = std::string(char_data.get_data());
                builder.append(kvp(key_str, cpp_str));
                break;
            }
            case Variant::ARRAY:
            {
                Array arr = value;
                
                builder.append(kvp(key_str, convert_array(arr)));
                break;
            }
            case Variant::DICTIONARY:
            {
                Dictionary value_dict = value;
                if(value_dict.has("$oid")) {
                    String id = value_dict.get("$oid", "");
                    try {
                        CharString id_data = id.ascii();
                        std::string cpp_str = std::string(id_data.get_data());
                        builder.append(kvp(key_str, bsoncxx::oid(cpp_str)));
                    } catch(const std::exception &e) {
                        ERR_PRINT("Invalid object id! " + id);
                    }
                } else {
                    builder.append(kvp(key_str, convert_dictionary_to_document(value)));
                }
                break;
            }
            default:
                WARN_PRINT("Unknown variant type in dictionary!");
                break;
        }
        
    }

    return builder.extract();
}

bsoncxx::array::value convert_array(Array array) {
    bsoncxx::builder::basic::array builder{};

    for(int i = 0; i < array.size(); i++) {
        auto value = array.get(i);

        switch(value.get_type()) {
            case Variant::NIL:
                //builder.append(kvp(key_str, nullptr));
                break;
            case Variant::BOOL:
                builder.append((bool)value);
                break;
            case Variant::INT:
                builder.append((int)value);
                break;
            case Variant::REAL:
                builder.append((float)value);
                break;
            case Variant::STRING:
            {
                String gd_str = value;
                CharString char_data = gd_str.utf8();
                std::string cpp_str = std::string(char_data.get_data());
                builder.append(cpp_str);
                break;
            }
            case Variant::ARRAY:
            {
                Array arr = value;
                
                builder.append(convert_array(arr));
                break;
            }
            case Variant::DICTIONARY:
                builder.append(convert_dictionary_to_document(value));
                break;
            default:
                WARN_PRINT("Unknown variant type in array!");
                break;
        }
    }

    return builder.extract();
}

Array convert_array(bsoncxx::array::view_or_value array_view) {
    auto arr = array_view.view();
    Array array;

    for(auto i = arr.begin(); i != arr.end(); i++) {
        switch(i->type()) {
            case bsoncxx::type::k_double:
                array.append((float)(double)i->get_double());
                break;
            case bsoncxx::type::k_utf8:
            {
                auto str = i->get_utf8().value.to_string();   
                String gd_str = String(str.c_str());

                array.append(gd_str);
                break;
            }
            case bsoncxx::type::k_document:
                array.append(convert_document_to_dictionary(i->get_document().view()));
                break;
            case bsoncxx::type::k_array:
                array.append(convert_array(i->get_array().value));
                break;
            // todo binary
            case bsoncxx::type::k_undefined:
                array.append(Variant());
                break;
            case bsoncxx::type::k_oid:
            {
                Dictionary oid_dict;
                auto oid = i->get_oid().value.to_string();
                oid_dict["$oid"] = String(oid.c_str());
                array.append(oid_dict);
                break;
            }
            case bsoncxx::type::k_bool:
                array.append((bool)i->get_bool());
                break;
            case bsoncxx::type::k_date:
                array.append(i->get_date().to_int64());
                break;
            case bsoncxx::type::k_null:
                array.append(Variant());
                break;
            // todo regex
            // todo dbpointer
            // todo code
            // todo symbol
            // todo codewscope
            case bsoncxx::type::k_int32:
                array.append((int)i->get_int32());
                break;
            case bsoncxx::type::k_int64:
                array.append((long)i->get_int64());
                break;
            case bsoncxx::type::k_timestamp:
                array.append(i->get_timestamp().timestamp);
                break;
            // todo decimal128
            default:
                WARN_PRINT(vformat("Unknown type in mongodb response! %d", (int)i->type()));
                break;
            
        }
    }

    return array;
}

Dictionary convert_document_to_dictionary(bsoncxx::document::view_or_value document) {
    Dictionary dict;
    auto view = document.view();

    for(auto i = view.begin(); i != view.end(); i++) {
        auto key = i->key().to_string();
        String gd_key = String(key.c_str());

        switch(i->type()) {
            case bsoncxx::type::k_double:
                dict[gd_key] = (float)(double)i->get_double();
                break;
            case bsoncxx::type::k_utf8:
            {
                auto str = i->get_utf8().value.to_string();   
                String gd_str = String(str.c_str());

                dict[gd_key] = gd_str;
                break;
            }
            case bsoncxx::type::k_document:
                dict[gd_key] = convert_document_to_dictionary(i->get_document().view());
                break;
            case bsoncxx::type::k_array:
                dict[gd_key] = convert_array(i->get_array().value);
                break;
            // todo binary
            case bsoncxx::type::k_undefined:
                dict[gd_key] = Variant();
                break;
            case bsoncxx::type::k_oid:
            {
                Dictionary oid_dict;
                auto oid = i->get_oid().value.to_string();
                oid_dict["$oid"] = String(oid.c_str());
                dict[gd_key] = oid_dict;
                break;
            }
            case bsoncxx::type::k_bool:
                dict[gd_key] = (bool)i->get_bool();
                break;
            case bsoncxx::type::k_date:
                dict[gd_key] = i->get_date().to_int64();
                break;
            case bsoncxx::type::k_null:
                dict[gd_key] = Variant();
                break;
            // todo regex
            // todo dbpointer
            // todo code
            // todo symbol
            // todo codewscope
            case bsoncxx::type::k_int32:
                dict[gd_key] = (int)i->get_int32();
                break;
            case bsoncxx::type::k_int64:
                dict[gd_key] = (long)i->get_int64();
                break;
            case bsoncxx::type::k_timestamp:
                dict[gd_key] = i->get_timestamp().timestamp;
                break;
            // todo decimal128
            default:
                WARN_PRINT(vformat("Unknown type in mongodb response! %d", (int)i->type()));
                break;
            
        }
    }

    return dict;
}