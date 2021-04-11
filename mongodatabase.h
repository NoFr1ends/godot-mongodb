#ifndef MONGODATABASE_H
#define MONGODATABASE_H

#include "core/reference.h"

#include "mongodb.h"

class MongoCollection;

class MongoDatabase : public Reference {
    GDCLASS(MongoDatabase, Reference);
    friend MongoCollection;
public:
    MongoDatabase();

    void create(Ref<MongoDB> db, String database);

protected:
    static void _bind_methods();

    Variant run_command(Dictionary command);
    Ref<MongoCollection> get_collection(String name);

private:
    Ref<MongoDB> m_db;
    String m_database;
};

#endif