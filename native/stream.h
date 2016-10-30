#ifndef __STREAM_H__
#define __STREAM_H__

#include "base.h"
#include "error.h"
#include "handle.h"
#include "callback.h"

#include <algorithm>

namespace native
{
    namespace base
    {

		template<typename T>
		struct uv_stream_task_t :public uv_stream_t,public promise_data_base_t<T>{
			using type = T;
		};
		template<typename T>
		struct uv_write_task_t :public uv_write_t, public promise_data_base_t<T> {
			using type = T;
		};
		template<typename T>
		struct uv_shutdown_task_t :public uv_shutdown_t, public promise_data_base_t<T> {
			using type = T;
		};
        class stream : public handle
        {
        public:
            template<typename T>
            stream(T* x)
                : handle(x)
            { }

			error accept(stream* client)
			{
				return uv_accept(get<uv_stream_t>(), client->get<uv_stream_t>());
			}



			task<error> listen(int backlog = 128) {

				PROMISE_DATA_HEAD(error);
				auto uv_stream = get<uv_stream_t>();
				uv_stream->data = &uv_req;

                int err = uv_listen(uv_stream, backlog, [](uv_stream_t* req, int status) {
					auto val = static_cast<promise_cur_data_t*>(req->data);
					val->set_value(status);
					val->resume();
                });
				co_await ret;
				return ret_value;
            }

            template<size_t min_alloc_size>
			task<std::tuple<std::string,ssize_t>> read_start()
            {

				PROMISE_DATA_HEAD(std::tuple<std::string,ssize_t>);
				auto uv_stream = get<uv_stream_t>();
				uv_stream->data = &uv_req;
				
				uv_read_start(uv_stream,
							[](uv_handle_t* h, size_t suggested_size, uv_buf_t*uvbuf) {
							auto val = static_cast<promise_cur_data_t*>(req->data);
							uvbuf->len = (std::max)(suggested_size, min_alloc_size);
							auto &str = std::get<0>(val->value_);
							str.resize(uvbuf->len);
							uvbuf->base = &str[0];
						},
							[](uv_stream_t* req, ssize_t nread, const uv_buf_t* uvbuf) {
							auto val = static_cast<promise_cur_data_t*>(req->data);
							auto &err = std::get<1>(val->value_);
							auto &str = std::get<0>(val->value_);
							err = nread;
							if (err >= 0) str.resize(err);
							val->resume();
                    });
				co_await ret;
				return ret_value;
            }

			task<std::tuple<std::string, ssize_t>> read_start()
			{
				std::tuple<std::string, ssize_t> ret_value = co_await read_start<0>();
				return ret_value;
			}

			int read_stop()
            {
                return uv_read_stop(get<uv_stream_t>());
            }

            // TODO: implement read2_start()

			task<error> write(const char* buf, int len)
            {
				PROMISE_REQ_HEAD(uv_write_task_t, error);

				uv_buf_t bufs[] = { uv_buf_t{ static_cast<size_t>(len), const_cast<char*>(buf) } };
				int err = uv_write(&uv_req, get<uv_stream_t>(), bufs, 1,
					[](uv_write_t* req, int status) {
					auto val = static_cast<promise_cur_data_t*>(req);
					val->set_value(status);
					val->resume();
				});

				co_await ret;
				return ret_value;
			}

			task<error> write(const std::string& buf)
            {
				return co_await write(buf.c_str(), static_cast<int>(buf.size()));
            }

			task<error> write(const std::vector<char>& buf)
            {
				return co_await write(&buf[0], static_cast<int>(buf.size()));
            }

            // TODO: implement write2()

			task<error> shutdown()
            {
				PROMISE_REQ_HEAD(uv_shutdown_task_t,error);
				auto uv_stream = get<uv_stream_t>();
				uv_stream->data = &uv_req;

                int err = uv_shutdown(&uv_req, get<uv_stream_t>(), [](uv_shutdown_t* req, int status) {
					auto val = static_cast<promise_cur_data_t*>(req->data);
					val->set_value(status);
					val->resume();
                });
				co_await ret;
				return ret_value;
			}
        };
    }
}

#endif
