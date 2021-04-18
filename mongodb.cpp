#include "mongodb.h"

#include "modules/regex/regex.h"
#include "core/io/marshalls.h"

#include "mongodatabase.h"
#include "query_result.h"
#include "bson.h"
#include "auth/scram_auth.h"

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

    // Parse options
    auto options_str = match->get_string("options");
    Dictionary options;
    if(!options_str.empty()) {
        options_str = options_str.substr(1); // remove question mark
        auto option_list = options_str.split("&");
        for(int i = 0; i < option_list.size(); i++) {
            auto option = option_list[i].split("=", true, 1);
            options[option[0]] = option.size() > 1 ? (Variant)option[1] : (Variant)true;
        }
    }

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

    // Check if we have auth details in the connection string
    if(!m_username.empty() && !m_password.empty()) {
        String auth_database = m_database;
        if(options.has("authSource")) auth_database = options["authSource"];

        if(auth_database.empty()) auth_database = "admin";
        auth(m_username, m_password, auth_database);
    }
}

void MongoDB::auth(String username, String password, String auth_database) {
    // FIXME: check which authentication mechanism should be used and is supported by the server
    Ref<ScramAuth> auth = memnew(ScramAuth());
    auth->prepare(username, password, auth_database);
    auth->auth(this);
}

Ref<MongoDatabase> MongoDB::get_database(String name) {
    Ref<MongoDatabase> database;
    database.instance();
    database->create(this, name);
    return database;
}

#define MAKE_ROOM(m_amount) \
	if (m_packet.size() < m_amount) m_packet.resize(m_amount)

constexpr int32_t message_header_size = 4 + 4 + 4 + 4;

void MongoDB::parse_msg(int32_t length, int32_t response_to) {
    auto flags = m_tcp->get_32();

    auto section = m_tcp->get_8();
    ERR_FAIL_COND_MSG(section != 0, "Currently only body sections supported");

    auto reply_length = length - message_header_size - 4 - 1;
    MAKE_ROOM(reply_length);
    auto status = m_tcp->get_data(&m_packet.write[0], reply_length);
    if(status != OK) {
        WARN_PRINT("Failed to read data (" + itos(status) + ")")
    }

    Dictionary reply;
    Bson::deserialize(&m_packet, reply, 0);
    
    auto result = m_pending[response_to];
    result->process_msg(reply);
}

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

    if(opcode == 1) {
        ERR_FAIL_MSG("Received old reply type which is unsupported");
    } else if(opcode == 2013) {
        parse_msg(length, response_to);
    } else {
        WARN_PRINT("Invalid response from server");
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

void MongoDB::execute_msg(Ref<ReplyListener> result, Dictionary document, int flags) {
    auto request_id = m_request_id++;
    int position = write_msg_header(request_id, 2013);

    // Flags
    MAKE_ROOM(position + 4);
    encode_uint32(flags, &m_packet.write[position]);
    position += 4;

    // Kind of section
    MAKE_ROOM(position + 1);
    m_packet.write[position] = 0;
    position += 1;

    // Query / Document
    position += Bson::serialize(document, &m_packet, position);

    // Length
    encode_uint32(position, &m_packet.write[0]);

    // Set query id to query result
    if(result.is_valid()) {
        result->set_request_id(request_id);
        m_pending.set({ request_id, result });
    }

    m_tcp->put_data(m_packet.ptr(), position);
}

void MongoDB::_bind_methods() {
    ClassDB::bind_method(D_METHOD("auth", "username", "password", "auth_database"), &MongoDB::auth);
    ClassDB::bind_method(D_METHOD("poll"), &MongoDB::poll);
    ClassDB::bind_method(D_METHOD("connect_database", "connection_uri"), &MongoDB::connect_database);
    ClassDB::bind_method(D_METHOD("get_database", "name"), &MongoDB::get_database);
    ClassDB::bind_method(D_METHOD("get_default_cursor_size"), &MongoDB::get_default_cursor_size);
    ClassDB::bind_method(D_METHOD("set_default_cursor_size", "cursor_size"), &MongoDB::set_default_cursor_size);

    ADD_PROPERTY(PropertyInfo(Variant::INT, "default_cursor_size"), "set_default_cursor_size", "get_default_cursor_size");
}