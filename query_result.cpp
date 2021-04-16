#include "query_result.h"

#include "mongodb.h"

QueryResult::QueryResult() {
    ERR_FAIL_MSG("Cannot instance QueryResult from script");
}

QueryResult::QueryResult(Ref<MongoDB> db, Dictionary request) : m_db(db), m_request(std::move(request)) {

}

QueryResult::~QueryResult() {
    if(m_cursor_id != 0 && m_db.is_valid()) {
        print_verbose("Free mongodb cursor " + itos(m_cursor_id));
        m_db->free_cursor(m_cursor_id);
    }
}

bool QueryResult::next() {
    if(m_request_id == 0) {
        // Not yet requested, so send query to the server
        m_db->execute_msg(this, m_request, 0);
        return true;
    }
    
    if(!has_more_data()) return false;

    Dictionary get_more;
    get_more["getMore"] = m_cursor_id;
    get_more["collection"] = m_request["find"];
    get_more["batchSize"] = m_db->get_default_cursor_size();
    get_more["$db"] = m_request["$db"];
    m_db->execute_msg(this, get_more, 0);
    return true;
}

void QueryResult::process_msg(Dictionary &reply) {
    if((int)reply["ok"] != 1) {
        // todo handle error reply
        ERR_FAIL_MSG("Received error from server: " + (String)reply["errmsg"]);
        emit_signal("completed", false);
        return;
    }

    // TODO: Improve a lot, we should maybe store what kind of reply we expect

    if(reply.has("value")) {
        m_result = reply["value"];
    } else if(reply.has("cursor")) {
        Dictionary cursor = reply["cursor"];
        m_cursor_id = cursor["id"];
        if(cursor.has("nextBatch")) {
            m_result = cursor["nextBatch"];
        } else {
            m_result = cursor["firstBatch"];
        }

        if(m_single_result) {
            Array results = m_result;
            m_result = results.empty() ? Variant() : results[0];
        }
    }

    emit_signal("completed", this);
}

void QueryResult::_bind_methods() {
    ClassDB::bind_method(D_METHOD("assign_result", "result"), &QueryResult::assign_result);
    ClassDB::bind_method(D_METHOD("get_result"), &QueryResult::get_result);
    ClassDB::bind_method(D_METHOD("has_more_data"), &QueryResult::has_more_data);
    ClassDB::bind_method(D_METHOD("next"), &QueryResult::next);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "data"), "assign_result", "get_result");
    ADD_SIGNAL(MethodInfo("completed"));
}