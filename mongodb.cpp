#include "mongodb.h"

#include "modules/regex/regex.h"
#include "core/io/marshalls.h"

#include "mongodatabase.h"
#include "query_result.h"
#include "bson.h"

MongoDB::MongoDB() {

}

IP_Address resolveHost(String host) {
    IP_Address ip;
	if (host.is_valid_ip_address()) {
		ip = host;
	} else {
		ip = IP::get_singleton()->resolve_hostname(host);
	}
    return ip;
}

void MongoDB::connect_database(String connection_uri) {
    // Parse connection uri
    ERR_FAIL_COND_MSG(!connection_uri.begins_with("mongodb:") && !connection_uri.begins_with("mongodb+srv:"), "Invalid connection url provided")
    // FIXME: mongodb+srv not implemented yet
    ERR_FAIL_COND_MSG(connection_uri.begins_with("mongodb+srv:"), "DNS connection strings are not supported yet")

    // Remove protocol path
    connection_uri = connection_uri.substr(10);
    
    // Parse the url with regex
    // FIXME: This is 100% not accurate and doesn't allow everything
    //        but it gets the job done for now
    Ref<RegEx> regex;
    regex.instance();
    regex->compile("^(?>(?<username>[a-zA-Z0-9\\-]*):(?<password>[a-zA-Z0-9\\-]*)@)?(?<host>[a-zA-Z0-9\\.\\-]*)(?>\\/(?<database>[a-zA-Z0-9]*))?(?<options>\\?.*)?$");
    auto match = regex->search(connection_uri);
    ERR_FAIL_COND_MSG(!match.is_valid(), "Failed to parse connection uri")

    m_host = match->get_string("host");
    m_database = match->get_string("database");
    m_username = match->get_string("username");
    m_password = match->get_string("password");

    // Check if we already have a tcp client
    if(m_tcp.is_valid()) {
        // Disconnect from the old connection
        m_tcp->disconnect_from_host();
    } else {
        // Create new connection
        m_tcp.instance();
    }

    // Connect to database host
    auto status = m_tcp->connect_to_host(resolveHost(m_host), 27017); // TODO: implement port
    ERR_FAIL_COND_MSG(status == FAILED, "Failed to connect to database")
}

Ref<MongoDatabase> MongoDB::get_database(String name) {
    Ref<MongoDatabase> database;
    database.instance();
    database->create(this, name);
    return database;
}

#define MAKE_ROOM(m_amount) \
	if (m_packet.size() < m_amount) m_packet.resize(m_amount)

void MongoDB::poll() {
    if(!m_tcp.is_valid()) {
        ERR_FAIL_MSG("connection not valid")
        return;
    }
    if(m_tcp->get_status() != StreamPeerTCP::STATUS_CONNECTED) {
        ERR_FAIL_MSG("not connected (" + itos(m_tcp->get_status()) + ")")
        return;
    }

    // Check if we have length data
    if(m_tcp->get_available_bytes() < 4) return;

    auto length = m_tcp->get_32();
    auto request_id = m_tcp->get_32();
    auto response_to = m_tcp->get_32();
    auto opcode = m_tcp->get_32();

    if(opcode != 1) {
        WARN_PRINT("Invalid response from server");
        return;
    }

    auto response_flags = m_tcp->get_32();
    auto cursor_id = m_tcp->get_64();
    auto starting_from = m_tcp->get_32();
    auto returned = m_tcp->get_32();

    auto documents_length = length - 4 - 4 - 4 - 4 - 4 - 8 - 4 - 4;

    MAKE_ROOM(documents_length);
    auto status = m_tcp->get_data(&m_packet.write[0], documents_length);
    if(status != OK) {
        WARN_PRINT("Failed to read data (" + itos(status) + ")")
    }

    if(!m_pending.has(response_to)) {
        WARN_PRINT("Received response for unknown query")
    } else {
        auto result = m_pending[response_to];
        result->set_cursor_id(cursor_id);

        auto single_result = result->get_single_result();

        int position = 0;

        if(!single_result) {
            Array documents;
            for(int i = 0; i < returned; i++) {
                Dictionary document;
                position = Bson::deserialize(&m_packet, document, position);
                documents.append(document);
            }
            result->assign_result(documents);
        } else {
            if(returned > 0) {
                Dictionary document;
                Bson::deserialize(&m_packet, document, position);
                result->assign_result(document);
            } else {
                result->assign_result(Variant()); // set result to null
            }
        }
        
        result->emit_signal("completed");
        m_pending.erase(response_to);
    }
}

int MongoDB::write_msg_header(int request_id, int opcode) {
    int position = 4;

    // Request ID
    MAKE_ROOM(position + 4);
    encode_uint32(request_id, &m_packet.write[position]);
    position += 4;

    // Response ID
    MAKE_ROOM(position + 4);
    encode_uint32(0, &m_packet.write[position]);
    position += 4;

    // OPCode
    MAKE_ROOM(position + 4);
    encode_uint32(opcode, &m_packet.write[position]);
    position += 4;

    return position;
}

void MongoDB::execute_query(String collection_name, int skip, int results, Dictionary &query, Ref<QueryResult> result) {
    auto request_id = m_request_id++;
    int position = write_msg_header(request_id, 2004);

    // TODO: Implement query flags
    // Flags
    MAKE_ROOM(position + 4);
    encode_uint32(0, &m_packet.write[position]);
    position += 4;

    // Collection Name
    CharString collection = collection_name.utf8();
    auto length = encode_cstring(collection.get_data(), nullptr);
    MAKE_ROOM(position + length);
    encode_cstring(collection.get_data(), &m_packet.write[position]);
    position += length;

    // Number to skip
    MAKE_ROOM(position + 4);
    encode_uint32(skip, &m_packet.write[position]);
    position += 4;

    // Number to return
    MAKE_ROOM(position + 4);
    encode_uint32(m_cursor_size, &m_packet.write[position]);
    position += 4;

    // Query
    position += Bson::serialize(query, &m_packet, position);

    // Write length of final packet to the start
    encode_uint32(position, &m_packet.write[0]);

    // Set query id to query result
    result->set_request_id(request_id);

    m_pending.set({ request_id, result });

    // Send query to server
    m_tcp->put_data(m_packet.ptr(), position);
}

void MongoDB::execute_update(String collection_name, int flags, Dictionary &selector, Dictionary &update) {
    auto request_id = m_request_id++;
    int position = write_msg_header(request_id, 2001);

    // always zero - reserved for future use
    MAKE_ROOM(position + 4);
    encode_uint32(0, &m_packet.write[position]);
    position += 4;

    // Collection Name
    CharString collection = collection_name.utf8();
    auto length = encode_cstring(collection.get_data(), nullptr);
    MAKE_ROOM(position + length);
    encode_cstring(collection.get_data(), &m_packet.write[position]);
    position += length;

    // Flags
    MAKE_ROOM(position + 4);
    encode_uint32(flags, &m_packet.write[position]);
    position += 4;

    // Selector / Query
    position += Bson::serialize(selector, &m_packet, position);

    // Update
    position += Bson::serialize(update, &m_packet, position);

    // Write length of final packet to the start
    encode_uint32(position, &m_packet.write[0]);

    m_tcp->put_data(m_packet.ptr(), position);
}

void MongoDB::get_more(Ref<QueryResult> result) {
    auto request_id = m_request_id++;
    int position = write_msg_header(request_id, 2005);

    // always zero
    MAKE_ROOM(position + 4);
    encode_uint32(0, &m_packet.write[position]);
    position += 4;

    // Collection Name
    CharString collection = result->get_collection_name().utf8();
    auto length = encode_cstring(collection.get_data(), nullptr);
    MAKE_ROOM(position + length);
    encode_cstring(collection.get_data(), &m_packet.write[position]);
    position += length;

    // number to return (0 = server default)
    MAKE_ROOM(position + 4);
    encode_uint32(m_cursor_size, &m_packet.write[position]);
    position += 4;

    // Cursor ID
    MAKE_ROOM(position + 8);
    encode_uint64(result->get_cursor_id(), &m_packet.write[position]);
    position += 8;

    // length
    encode_uint32(position, &m_packet.write[0]);

    result->set_request_id(request_id);

    m_pending.set({ request_id, result });

    m_tcp->put_data(m_packet.ptr(), position);
}

void MongoDB::free_cursor(int64_t cursor_id) {
    auto request_id = m_request_id++;
    int position = write_msg_header(request_id, 2007);

    // always zero
    MAKE_ROOM(position + 4);
    encode_uint32(0, &m_packet.write[position]);
    position += 4;

    // number of cursors to free
    MAKE_ROOM(position + 4);
    encode_uint32(1, &m_packet.write[position]);
    position += 4;

    // cursor to remove
    MAKE_ROOM(position + 8);
    encode_uint64(cursor_id, &m_packet.write[position]);
    position += 8;

    // length
    encode_uint32(position, &m_packet.write[0]);

    m_tcp->put_data(m_packet.ptr(), position);
}

void MongoDB::_bind_methods() {
    ClassDB::bind_method(D_METHOD("poll"), &MongoDB::poll);
    ClassDB::bind_method(D_METHOD("connect_database", "connection_uri"), &MongoDB::connect_database);
    ClassDB::bind_method(D_METHOD("get_database", "name"), &MongoDB::get_database);
    ClassDB::bind_method(D_METHOD("get_default_cursor_size"), &MongoDB::get_default_cursor_size);
    ClassDB::bind_method(D_METHOD("set_default_cursor_size", "cursor_size"), &MongoDB::set_default_cursor_size);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "default_cursor_size"), "set_default_cursor_size", "get_default_cursor_size");
}