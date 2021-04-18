#pragma once

#include "core/dictionary.h"

class ReplyListener: public Reference {
    GDCLASS(ReplyListener, Reference);
public:
    virtual void set_request_id(int request_id) = 0;
    virtual void process_msg(Dictionary &reply) = 0;
};