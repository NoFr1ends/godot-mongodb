#ifndef BSON_HELPER_H
#define BSON_HELPER_H

#include <bsoncxx/builder/basic/document.hpp>

#include "core/variant.h"
#include "core/dictionary.h"

bsoncxx::document::value convert_dictionary_to_document(Dictionary dict);

bsoncxx::array::value convert_array(Array array);
Array convert_array(bsoncxx::array::view_or_value array_view);

Dictionary convert_document_to_dictionary(bsoncxx::document::view_or_value document);

#endif