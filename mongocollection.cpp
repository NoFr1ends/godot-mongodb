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
    Dictionary request;
    request["insert"] = m_collection;
    request["$db"] = m_database->m_database;

    Array documents;
    documents.append(document);

    request["documents"] = documents;

    Ref<QueryResult> result = memnew(QueryResult(m_database->m_db, request));
    result->next();
    return result;
}

Variant MongoCollection::find_one(Dictionary filter) {
    Dictionary request;
    request["find"] = m_collection;
    request["filter"] = filter;
    request["limit"] = 1;
    request["singleBatch"] = true;
    request["$db"] = m_database->m_database;

    Ref<QueryResult> result = memnew(QueryResult(m_database->m_db, request));
    result->set_single_result(true);
    result->next();
    return result;
}

Variant MongoCollection::find(Dictionary filter) {
    Dictionary request;
    request["find"] = m_collection;
    request["filter"] = filter;
    request["batchSize"] = m_database->m_db->get_default_cursor_size();
    request["$db"] = m_database->m_database;
    
    Ref<QueryResult> result = memnew(QueryResult(m_database->m_db, request));
    return result;
}

Variant MongoCollection::find_one_and_update(Dictionary filter, Dictionary update, bool after) {
    Dictionary request;
    request["findAndModify"] = m_collection;
    request["$db"] = m_database->m_database;
    request["query"] = filter;
    request["update"] = update;
    request["new"] = after;

    Ref<QueryResult> result = memnew(QueryResult(m_database->m_db, request));
    result->next();
    return result;
}

Variant MongoCollection::update_one(Dictionary filter, Dictionary update) {
    Dictionary request;
    request["update"] = m_collection;
    request["$db"] = m_database->m_database;

    Array updates;
    Dictionary update1 = Dictionary();
    update1["q"] = filter;
    update1["u"] = update;
    update1["multi"] = false;
    
    updates.append(update1);
    request["updates"] = updates;

    Ref<QueryResult> result = memnew(QueryResult(m_database->m_db, request));
    result->next(); // execute
    return result;
}

Variant MongoCollection::update_many(Dictionary filter, Dictionary update) {
    Dictionary request;
    request["update"] = m_collection;
    request["$db"] = m_database->m_database;

    Array updates;
    Dictionary update1 = Dictionary();
    update1["q"] = filter;
    update1["u"] = update;
    update1["multi"] = true;
    
    updates.append(update1);
    request["updates"] = updates;

    Ref<QueryResult> result = memnew(QueryResult(m_database->m_db, request));
    result->next(); // execute
    return result;
}

Variant MongoCollection::delete_one(Dictionary filter) {
    Dictionary request;
    request["delete"] = m_collection;
    request["$db"] = m_database->m_database;

    Array deletes;
    Dictionary delete1 = Dictionary();
    delete1["q"] = filter;
    delete1["limit"] = 1;
    
    deletes.append(delete1);
    request["deletes"] = deletes;

    Ref<QueryResult> result = memnew(QueryResult(m_database->m_db, request));
    result->next(); // execute
    return result;
}

Variant MongoCollection::delete_many(Dictionary filter) {
    Dictionary request;
    request["delete"] = m_collection;
    request["$db"] = m_database->m_database;

    Array deletes;
    Dictionary delete1 = Dictionary();
    delete1["q"] = filter;
    delete1["limit"] = 1;
    
    deletes.append(delete1);
    request["deletes"] = deletes;

    Ref<QueryResult> result = memnew(QueryResult(m_database->m_db, request));
    result->next(); // execute
    return result;
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