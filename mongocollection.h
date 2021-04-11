#ifndef MONGOCOLLECTION_H
#define MONGOCOLLECTION_H

#include "core/reference.h"

class MongoDatabase;

class MongoCollection : public Reference {
    GDCLASS(MongoCollection, Reference);

public:
    MongoCollection();

    void create(Ref<MongoDatabase> database, String collection);
    String full_collection_name();

protected:
    static void _bind_methods();

    Variant insert_one(Dictionary document);

    Variant find_one(Dictionary filter);
    Variant find(Dictionary filter);

    Variant find_one_and_update(Dictionary filter, Dictionary update);

    bool update_one(Dictionary filter, Dictionary update);
    bool update_many(Dictionary filter, Dictionary update);

    bool delete_one(Dictionary filter);
    bool delete_many(Dictionary filter);

private:
    Ref<MongoDatabase> m_database;
    String m_collection;
};

#endif