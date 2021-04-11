#include "mongodatabase.h"

#include "mongocollection.h"

MongoDatabase::MongoDatabase() {
}

void MongoDatabase::create(Ref<MongoDB> db, String database) {
    m_db = db;
    m_database = database;
}

/*Variant MongoDatabase::run_command(Dictionary command) {
    auto bson_doc = convert_dictionary_to_document(command);
    auto str = bsoncxx::to_json(bson_doc.view());
    
    try {
        auto response = m_database.run_command(bson_doc.view());

        return convert_document_to_dictionary(response.view());
    } catch (mongocxx::operation_exception &e) {
        return false;
    }
}*/

Ref<MongoCollection> MongoDatabase::get_collection(String name) {
    Ref<MongoCollection> ref;
    ref.instance();

    ref->create(this, name);

    return ref;
}

void MongoDatabase::_bind_methods() {
    //ClassDB::bind_method(D_METHOD("run_command", "command"), &MongoDatabase::run_command);
    ClassDB::bind_method(D_METHOD("get_collection", "collection"), &MongoDatabase::get_collection);
}