#include "query_result.h"

#include "mongodb.h"

QueryResult::QueryResult() {
    ERR_FAIL_MSG("Cannot instance QueryResult from script");
}

QueryResult::QueryResult(Ref<MongoDB> db) : m_db(db) {

}

QueryResult::~QueryResult() {
    if(m_cursor_id != 0 && m_db.is_valid()) {
        print_verbose("Free mongodb cursor " + itos(m_cursor_id));
        m_db->free_cursor(m_cursor_id);
    }
}

void QueryResult::_bind_methods() {
    ClassDB::bind_method(D_METHOD("assign_result", "result"), &QueryResult::assign_result);
    ClassDB::bind_method(D_METHOD("get_result"), &QueryResult::get_result);

    ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "data"), "assign_result", "get_result");
    ADD_SIGNAL(MethodInfo("completed"));
}