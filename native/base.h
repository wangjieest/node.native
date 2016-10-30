#ifndef __BASE_H__
#define __BASE_H__

#include <cassert>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <functional>
#include <map>
#include <algorithm>
#include <list>
#include <set>
#include <tuple>
#include "../libuv/include/uv.h"
#include "../../coroutine_tasks/include/coroutine_tasks.hpp"

namespace native
{

    namespace internal
    {
        enum uv_callback_id
        {
            uv_cid_close = 0,
            uv_cid_listen,
            uv_cid_read_start,
            uv_cid_write,
            uv_cid_shutdown,
            uv_cid_connect,
            uv_cid_connect6,
            uv_cid_max
        };
    }


	using namespace coroutine_tasks;


	template<typename T>
	struct promise_data_base_t{
		using type = T;
		promise_handle<> prom_;
		T value_;
		void resume() { prom_.resume(); }
		template<typename ...Args>
		void set_value(Args&...args) {
			value_ = T{ std::move(args)... };
		}
		void set_value(T&v) { value_ = std::move(v); }
	};

#define PROMISE_DATA_HEAD(...) \
		using promise_cur_data_t = promise_data_base_t<__VA_ARGS__>;\
		using data_type = typename promise_cur_data_t::type;\
		promise_cur_data_t uv_req;\
		auto& ret_value = uv_req.value_;\
		auto ret = make_task<void>();\
		uv_req.prom_ = ret.get_promise_handle();

#define PROMISE_REQ_HEAD(REQTYPE,...) \
		using promise_cur_data_t = REQTYPE<__VA_ARGS__>;\
		using data_type = typename promise_cur_data_t::type;\
		promise_cur_data_t uv_req;\
		auto ret = make_task<void>();\
		uv_req.prom_ = ret.get_promise_handle();\
		auto& ret_value = uv_req.value_; 

}

#endif
