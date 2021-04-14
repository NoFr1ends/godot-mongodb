#include "mongocollection.h"

#include "mongodatabase.h"
#include "query_result.h"

MongoCollection::MongoCollection() {}

void MongoCollection::create(Ref<MongoDatabase> database, String collection) {
    m_database = database;
    m_collection = collection;
}

String MongoCollection::full_collection_name() {
    return m_database->m_database + "." + m_collection;
}

Variant MongoCollection::insert_one(Dictionary document) {
    /*try {
        auto result = m_collection.insert_one(convert_dictionary_to_document(document));
        if(result) {
            auto inserted_id = result->inserted_id();
            
            Dictionary oid_dict;
            auto oid = inserted_id.get_oid().value.to_string();
            oid_dict["$oid"] = String(oid.c_str());

            return oid_dict;
        } else {
            return false;
        }
    } catch(mongocxx::operation_exception &e) {
        ERR_PRINT(e.what());
        return false;
    }*/
    return Variant();
}

Variant MongoCollection::find_one(Dictionary filter) {
    Ref<QueryResult> result = memnew(QueryResult(m_database->m_db, full_collection_name(), filter));
    result->set_single_result(true);

    m_database->m_db->execute_query(
        full_collection_name(),
        0,
        -1, /* close cursor automatically */
        filter,
        result
    );

    return result;
}

Variant MongoCollection::find(Dictionary filter) {
    Ref<QueryResult> result = memnew(QueryResult(m_database->m_db, full_collection_name(), filter));
    return result;
}

Variant MongoCollection::find_one_and_update(Dictionary filter, Dictionary update) {
    /*try {
        mongocxx::options::find_one_and_update options;
        options.return_document(mongocxx::options::return_document::k_after);
        auto result = m_collection.find_one_and_update(
            convert_dictionary_to_document(filter), 
            convert_dictionary_to_document(update),
            options
        );
        if(result) {
            return convert_document_to_dictionary((*result).view());
        } else {
            return Variant();
        }
    } catch(mongocxx::exception &e) {
        ERR_PRINT(e.what());
        return false;
    }*/
    return Variant();
}

bool MongoCollection::update_one(Dictionary filter, Dictionary update) {
    /*try {
        m_collection.update_one(convert_dictionary_to_document(filter), convert_dictionary_to_document(update));
        return true;
    } catch(mongocxx::exception &e) {
        ERR_PRINT(e.what());
        return false;
    }*/
    return false;
}

bool MongoCollection::update_many(Dictionary filter, Dictionary update) {
    /*try {
        m_collection.update_many(convert_dictionary_to_document(filter), convert_dictionary_to_document(update));
        return true;
    } catch(mongocxx::exception &e) {
        ERR_PRINT(e.what());
        return false;
    }*/
    return false;
}

bool MongoCollection::delete_one(Dictionary filter) {
    /*try {
        m_collection.delete_one(convert_dictionary_to_document(filter));
        return true;
    } catch(mongocxx::exception &e) {
        ERR_PRINT(e.what());
        return false;
    }*/
    return false;
}

bool MongoCollection::delete_many(Dictionary filter) {
    /*try {
        m_collection.delete_many(convert_dictionary_to_document(filter));
        return true;
    } catch(mongocxx::exception &e) {
        ERR_PRINT(e.what());
        return false;
    }*/
    return false;
}

void MongoCollection::_bind_methods() {
    ClassDB::bind_method(D_METHOD("insert_one", "document"), &MongoCollection::insert_one);

    ClassDB::bind_method(D_METHOD("find_one", "filter"), &MongoCollection::find_one);
    ClassDB::bind_method(D_METHOD("find", "filter"), &MongoCollection::find);

    ClassDB::bind_method(D_METHOD("find_one_and_update", "filter", "update"), &MongoCollection::find_one_and_update);

    ClassDB::bind_method(D_METHOD("update_one", "filter", "update"), &MongoCollection::update_one);
    ClassDB::bind_method(D_METHOD("update_many", "filter", "update"), &MongoCollection::update_many);

    ClassDB::bind_method(D_METHOD("delete_one", "filter"), &MongoCollection::delete_one);
    ClassDB::bind_method(D_METHOD("delete_many", "filter"), &MongoCollection::delete_many);
}