#pragma once

#include "../mongodb.h"

class AuthProvider {
public:
    virtual void prepare(String username, String password, String database) = 0;
    virtual void auth(Ref<MongoDB> mongo) = 0;
};