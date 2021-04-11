/* register_types.cpp */

#include "register_types.h"
#include "core/class_db.h"

#include "mongodb.h"
#include "mongodatabase.h"
#include "mongocollection.h"
#include "query_result.h"

void register_mongodb_types() {
    ClassDB::register_class<MongoDB>();
    ClassDB::register_class<MongoDatabase>();
    ClassDB::register_class<MongoCollection>();
    ClassDB::register_class<QueryResult>();
}

void unregister_mongodb_types() {
}

