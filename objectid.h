#pragma once

#include "core/reference.h"
#include "core/pool_vector.h"

class MongoObjectID: public Reference {
    GDCLASS(MongoObjectID, Reference);

public:
    MongoObjectID();
    ~MongoObjectID();

    static void _bind_methods();

    uint8_t *get_data() { return m_data; }

    String str();
    void from_string(String str);
    String to_string() override;

    static void initialize();
    
private:
    uint8_t *m_data;

    static uint32_t m_counter;
    static RandomPCG m_random;
};