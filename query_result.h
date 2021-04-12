#pragma once

#include "core/reference.h"

class MongoDB;

class QueryResult : public Reference {
    GDCLASS(QueryResult, Reference);

public:
    QueryResult();
    QueryResult(Ref<MongoDB> db);
    ~QueryResult();

    void set_request_id(int request_id) { m_request_id = request_id; }
    void set_cursor_id(int64_t cursor_id) { m_cursor_id = cursor_id; }

    void assign_result(Variant result) { m_result = result; }
    Variant get_result() { return m_result; }

    void set_single_result(bool single_result) { m_single_result = single_result; }
    bool get_single_result() { return m_single_result; }

protected:
    static void _bind_methods();

private:
    int m_request_id = { 0 };
    int64_t m_cursor_id = { 0 };
    bool m_single_result = { false };
    Variant m_result;

    Ref<MongoDB> m_db;
};