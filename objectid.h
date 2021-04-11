#pragma once

#include "core/reference.h"
#include "core/pool_vector.h"

class MongoObjectID: public Reference {
    GDCLASS(MongoObjectID, Reference);

public:
    MongoObjectID(PoolByteArray &data);

    static void _bind_methods();

    String to_string() override;
    
private:
    PoolByteArray m_data;
};