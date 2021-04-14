#include "query_result.h"

#include "mongodb.h"

QueryResult::QueryResult() {
    ERR_FAIL_MSG("Cannot instance QueryResult from script");
}

QueryResult::QueryResult(Ref<MongoDB> db, String collection_name, Dictionary filter) : m_db(db), m_full_collection_name(collection_name), m_filter(std::move(filter)) {

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
        m_db->execute_query(
            m_full_collection_name,
            0,
            0,
            m_filter,
            this
        );
        return true;
    }
    
    if(!has_more_data()) return false;

    m_db->get_more(this);
    return true;
}

void QueryResult::_bind_methods() {
    ClassDB::bind_method(D_METHOD("assign_result", "result"), &QueryResult::assign_result);
    ClassDB::bind_method(D_METHOD("get_result"), &QueryResult::get_result);
    ClassDB::bind_method(D_METHOD("has_more_data"), &QueryResult::has_more_data);
    ClassDB::bind_method(D_METHOD("next"), &QueryResult::next);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "data"), "assign_result", "get_result");
    ADD_SIGNAL(MethodInfo("completed"));
}