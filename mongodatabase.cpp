#include "mongodatabase.h"

#include "mongocollection.h"

MongoDatabase::MongoDatabase() {
}

void MongoDatabase::create(Ref<MongoDB> db, String database) {
    m_db = db;
    m_database = database;
}

Ref<MongoCollection> MongoDatabase::get_collection(String name) {
    Ref<MongoCollection> ref;
    ref.instance();

    ref->create(this, name);

    return ref;
}

Ref<QueryResult> MongoDatabase::connection_status() {
    Dictionary request;
    request["connectionStatus"] = 1;
    request["showPrivileges"] = false;
    request["$db"] = m_database;

    Ref<QueryResult> query = memnew(QueryResult(m_db, request));
    query->next();
    return query;
}

void MongoDatabase::_bind_methods() {
    ClassDB::bind_method(D_METHOD("connection_status"), &MongoDatabase::connection_status);
    ClassDB::bind_method(D_METHOD("get_collection", "collection"), &MongoDatabase::get_collection);
}