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
        class stream : public handle
        {
        public:
            template<typename T>
            stream(T* x)
                : handle(x)
            { }

			error listen(std::function<void(error)>&& callback, int backlog=128)
            {
                callbacks::store(get()->data, native::internal::uv_cid_listen, std::move(callback));
                return uv_listen(get<uv_stream_t>(), backlog, [](uv_stream_t* s, int status) {
                    callbacks::invoke<decltype(callback)>(s->data, native::internal::uv_cid_listen, status);
                });
            }

			error accept(stream* client)
            {
                return uv_accept(get<uv_stream_t>(), client->get<uv_stream_t>());
            }

			error read_start(std::function<void(const char* buf, ssize_t len)>&& callback)
            {
                return read_start<0>(std::move(callback));
            }

            template<size_t max_alloc_size>
			error read_start(std::function<void(const char* buf, ssize_t len)>&& callback)
            {
                callbacks::store(get()->data, native::internal::uv_cid_read_start, std::move(callback));

                return uv_read_start(get<uv_stream_t>(),
                    [](uv_handle_t*, size_t suggested_size, uv_buf_t*uvbuf){
                        uvbuf->len = (std::max)(suggested_size, max_alloc_size);
                        uvbuf->base = new char[uvbuf->len];
                    },
                    [](uv_stream_t* s, ssize_t nread, const uv_buf_t* uvbuf){
                        if(nread < 0)
                        {
                            callbacks::invoke<decltype(callback)>(s->data, native::internal::uv_cid_read_start, nullptr, nread);
                        }
                        else if(nread >= 0)
                        {
                            callbacks::invoke<decltype(callback)>(s->data, native::internal::uv_cid_read_start, uvbuf->base, nread);
                        }
                        delete uvbuf->base;
                    });
            }

			error read_stop()
            {
                return uv_read_stop(get<uv_stream_t>());
            }

            // TODO: implement read2_start()

			error write(const char* buf, int len, std::function<void(error)>&& callback)
            {
                uv_buf_t bufs[] = { uv_buf_t { static_cast<size_t>(len), const_cast<char*>(buf) } };
                callbacks::store(get()->data, native::internal::uv_cid_write, std::move(callback));
                return uv_write(new uv_write_t, get<uv_stream_t>(), bufs, 1, [](uv_write_t* req, int status) {
                    callbacks::invoke<decltype(callback)>(req->handle->data, native::internal::uv_cid_write, status);
                    delete req;
                });
            }

			error write(const std::string& buf, std::function<void(error)>&& callback)
            {
                uv_buf_t uvbuf = { static_cast<uint32_t>(buf.length()), const_cast<char*>(buf.c_str()) };
                callbacks::store(get()->data, native::internal::uv_cid_write, std::move(callback));
                return uv_write(new uv_write_t, get<uv_stream_t>(), &uvbuf, 1, [](uv_write_t* req, int status) {
                    callbacks::invoke<decltype(callback)>(req->handle->data, native::internal::uv_cid_write, status);
                    delete req;
                });
            }

			error write(const std::vector<char>& buf, std::function<void(error)>&& callback)
            {
                uv_buf_t uvbuf = { static_cast<uint32_t>(buf.size()), const_cast<char*>(&buf[0]) };
                callbacks::store(get()->data, native::internal::uv_cid_write, std::move(callback));
                return uv_write(new uv_write_t, get<uv_stream_t>(), &uvbuf, 1, [](uv_write_t* req, int status) {
                    callbacks::invoke<decltype(callback)>(req->handle->data, native::internal::uv_cid_write, status);
                    delete req;
                });
            }

            // TODO: implement write2()

			error shutdown(std::function<void(error)>&& callback)
            {
                callbacks::store(get()->data, native::internal::uv_cid_shutdown, std::move(callback));
                return uv_shutdown(new uv_shutdown_t, get<uv_stream_t>(), [](uv_shutdown_t* req, int status) {
                    callbacks::invoke<decltype(callback)>(req->handle->data, native::internal::uv_cid_shutdown, status);
                    delete req;
                });
            }
        };
    }
}

#endif
