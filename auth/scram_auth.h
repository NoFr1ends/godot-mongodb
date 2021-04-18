#pragma once

#include "core/reference.h"

#include "auth_provider.h"
#include "../mongodb.h"
#include "../reply_listener.h"

class ScramAuth : public AuthProvider, public ReplyListener {
    GDCLASS(ScramAuth, ReplyListener)
public:
    void prepare(String username, String password, String database) override;
    void auth(Ref<MongoDB> mongo) override;

    void set_request_id(int request_id) override { m_request_id = request_id; }
    void process_msg(Dictionary &reply) override;

private:
    void send_init(Ref<MongoDB> mongo);
    String password_digest();

    String first_message();

private:
    PoolByteArray m_nonce;
    String m_username;
    String m_password;
    String m_database;
    Ref<MongoDB> m_mongo;
    PoolByteArray m_server_signature;
    int m_request_id;
};