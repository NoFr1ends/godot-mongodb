#ifndef MONGODB_H
#define MONGODB_H

#include "core/reference.h"
#include "core/hash_map.h"
#include "core/io/stream_peer_tcp.h"

#include "query_result.h"

class MongoDatabase;

class MongoDB : public Reference {
    GDCLASS(MongoDB, Reference);

public:
    void execute_query(String collection_name, int skip, int results, Dictionary &query, Ref<QueryResult> result);
    void get_more(Ref<QueryResult> result);
    void free_cursor(int64_t cursor_id);

protected:
    static void _bind_methods();

    void connect_database(String connection_uri);
    Ref<MongoDatabase> get_database(String name);

    int get_default_cursor_size() { return m_cursor_size; }
    void set_default_cursor_size(int cursor_size) { m_cursor_size = cursor_size; } 

    void poll();

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
    HashMap<int, Ref<QueryResult>> m_pending;

public:
    MongoDB();
};

#endif