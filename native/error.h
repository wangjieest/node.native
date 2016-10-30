#ifndef __ERROR_H__
#define __ERROR_H__

#include"base.h"

namespace native
{
    class exception
    {
    public:
        exception(const std::string& message)
            : message_(message)
        {}

        virtual ~exception() {}

        const std::string& message() const { return message_; }

    private:
        std::string message_;
    };

    class error
    {
    public:
        error() : uv_err_() {}
        error(int e) : uv_err_(e) {}
        ~error() = default;
		error(const error&) = default;
		error& operator=(const error&) = default;
    public:
        operator bool() { return uv_err_ != 0; }

        int code() const { return uv_err_; }
        const char* name() const { return uv_err_name(uv_err_); }
        const char* str() const { return uv_strerror(uv_err_); }

        int uv_err_;
    };
}


#endif
