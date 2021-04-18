#ifndef MONGODB_H
#define MONGODB_H

#include "core/reference.h"
#include "core/hash_map.h"
#include "core/io/stream_peer_tcp.h"

#include "reply_listener.h"

class MongoDatabase;

class MongoDB : public Reference {
    GDCLASS(MongoDB, Reference);

public:
    enum UpdateFlags {
        UPSERT = 1,
        MULTI_UPDATE = 2
    };

public:
    void auth(String username, String password, String auth_database);

    void free_cursor(int64_t cursor_id);
    void execute_msg(Ref<ReplyListener> result, Dictionary document, int flags);

    int get_default_cursor_size() { return m_cursor_size; }
    void set_default_cursor_size(int cursor_size) { m_cursor_size = cursor_size; } 

protected:
    static void _bind_methods();

    void connect_database(String connection_uri);
    Ref<MongoDatabase> get_database(String name);

    void poll();

private:
    int write_msg_header(int request_id, int opcode);
    void parse_msg(int32_t length, int32_t response_to);

private:
    Ref<StreamPeerTCP> m_tcp;

    String m_host;
    String m_database;
    String m_username;
    String m_password;
    String m_auth_database;

    int m_request_id = { 1 };

    int m_cursor_size = { 1000 };

    Vector<uint8_t> m_packet;
    HashMap<int, Ref<ReplyListener>> m_pending;

public:
    MongoDB();
};

#endif