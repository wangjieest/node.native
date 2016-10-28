#ifndef __FS_H__
#define __FS_H__

#include "base.h"
#include <functional>
#include <algorithm>
#include <fcntl.h>
#include "callback.h"

namespace native
{
    // TODO: implement functions that accept loop pointer as extra argument.
    namespace fs
    {
        typedef uv_file file_handle;

        static const int read_only = O_RDONLY;
        static const int write_only = O_WRONLY;
        static const int read_write = O_RDWR;
        static const int append = O_APPEND;
        static const int create = O_CREAT;
        static const int excl = O_EXCL;
        static const int truncate = O_TRUNC;
#ifdef O_NOFOLLOW
        static const int no_follow = O_NOFOLLOW;
#endif
#ifdef O_DIRECTORY
        static const int directory = O_DIRECTORY;
#endif
#ifdef O_NOATIME
        static const int no_access_time = O_NOATIME;
#endif
#ifdef O_LARGEFILE
        static const int large_large = O_LARGEFILE;
#endif

        namespace internal
        {
            template<typename callback_t>
            uv_fs_t* create_req(callback_t&& callback, void* data=nullptr)
            {
                auto req = new uv_fs_t;
                req->data = new callbacks(1);
                assert(req->data);
                callbacks::store(req->data, 0, std::forward<callback_t>(callback), data);

                return req;
            }

            template<typename callback_t, typename ...A>
            typename std::result_of<callback_t(A...)>::type invoke_from_req(uv_fs_t* req, A&& ... args)
            {
                return callbacks::invoke<callback_t>(req->data, 0, std::forward<A>(args)...);
            }

            template<typename callback_t, typename data_t>
            data_t* get_data_from_req(uv_fs_t* req)
            {
                return reinterpret_cast<data_t*>(callbacks::get_data<callback_t>(req->data, 0));
            }

            template<typename callback_t, typename data_t>
            void delete_req(uv_fs_t* req)
            {
                delete reinterpret_cast<data_t*>(callbacks::get_data<callback_t>(req->data, 0));
                delete reinterpret_cast<callbacks*>(req->data);
                uv_fs_req_cleanup(req);
                delete req;
            }

            template<typename callback_t, typename data_t>
            void delete_req_arr_data(uv_fs_t* req)
            {
                delete[] reinterpret_cast<data_t*>(callbacks::get_data<callback_t>(req->data, 0));
                delete reinterpret_cast<callbacks*>(req->data);
                uv_fs_req_cleanup(req);
                delete req;
            }

            inline void delete_req(uv_fs_t* req)
            {
                delete reinterpret_cast<callbacks*>(req->data);
                uv_fs_req_cleanup(req);
                delete req;
            }

            struct rte_context
            {
                fs::file_handle file;
                const static int buflen = 32;
                char buf[buflen];
                std::string result;
            };

            template<typename callback_t>
            void rte_cb(uv_fs_t* req)
            {
                assert(req->fs_type == UV_FS_READ);

                auto ctx = reinterpret_cast<rte_context*>(callbacks::get_data<callback_t>(req->data, 0));
                if(req->result < 0)
                {
                    // system error
                    invoke_from_req<callback_t>(req, std::string(), error(req->result));
                    delete_req<callback_t, rte_context>(req);
                }
                else if(req->result == 0)
                {
                    // EOF
                    invoke_from_req<callback_t>(req, ctx->result, error());
                    delete_req<callback_t, rte_context>(req);
                }
                else
                {
                    ctx->result.append(std::string(ctx->buf, req->result));

                    uv_fs_req_cleanup(req);
                    uv_buf_t uvbuf = { rte_context::buflen, ctx->buf };
					error err = uv_fs_read(uv_default_loop(), req, ctx->file, &uvbuf, 1, ctx->result.length(), rte_cb<callback_t>);
                    if (err)
                    {
                        // failed to initiate uv_fs_read():
                        invoke_from_req<callback_t>(req, std::string(), error(err));
                        delete_req<callback_t, rte_context>(req);
                    }
                }
            }
        }

        inline int open(const std::string& path, int flags, int mode, std::function<void(native::fs::file_handle fd, error e)>&& callback)
        {
            auto req = internal::create_req(std::move(callback));
			error err = uv_fs_open(uv_default_loop(), req, path.c_str(), flags, mode, [](uv_fs_t* req) {
                assert(req->fs_type == UV_FS_OPEN);

                if(req->result < 0) internal::invoke_from_req<decltype(callback)>(req, file_handle(-1), error(req->result));
                else internal::invoke_from_req<decltype(callback)>(req, req->result, error(req->result));

                internal::delete_req(req);
            });
            if (err)
            {
                // failed to initiate uv_fs_open()
                internal::delete_req(req);
            }
            return err;
        }

        inline int read(file_handle fd, size_t len, off_t offset, std::function<void(const std::string& str, error e)>&& callback)
        {
            uv_buf_t uvbuf = { static_cast<size_t>(len), new char[len] };
            auto req = internal::create_req(std::move(callback), uvbuf.base);
			error err = uv_fs_read(uv_default_loop(), req, fd, &uvbuf, 1, offset, [](uv_fs_t* req) {
                assert(req->fs_type == UV_FS_READ);
                if(req->result < 0)
                {
                    // system error
                    internal::invoke_from_req<decltype(callback)>(req, std::string(), error(req->result));
                }
                else if(req->result == 0)
                {
                    // EOF
                    internal::invoke_from_req<decltype(callback)>(req, std::string(), error(UV_EOF));
                }
                else
                {
                    auto buf = internal::get_data_from_req<decltype(callback), char>(req);
                    internal::invoke_from_req<decltype(callback)>(req, std::string(buf, req->result), error());
                }

                internal::delete_req_arr_data<decltype(callback), char>(req);
            });
            if (err)
            {
                // failed to initiate uv_fs_read()
                internal::delete_req_arr_data<decltype(callback), char>(req);
            }
            return err;
        }

        inline int write(file_handle fd, const char* buf, size_t len, off_t offset, std::function<void(int nwritten, error e)>&& callback)
        {
            uv_buf_t uvbuf = { len, const_cast<char*>(buf) };
            auto req = internal::create_req(std::move(callback));
            // TODO: const_cast<> !!
			error err = uv_fs_write(uv_default_loop(), req, fd, &uvbuf,  1, offset, [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_WRITE);

                if(req->result < 0)
                {
                    internal::invoke_from_req<decltype(callback)>(req, 0, error(req->result));
                }
                else
                {
                    internal::invoke_from_req<decltype(callback)>(req, req->result, error());
                }

                internal::delete_req(req);
            });
            if (err)
            {
                // failed to initiate uv_fs_write()
                internal::delete_req(req);
            }
            return err;
        }

        inline error read_to_end(file_handle fd, std::function<void(const std::string& str, error e)>&& callback)
        {
            auto ctx = new internal::rte_context;
            ctx->file = fd;
            auto req = internal::create_req(std::move(callback), ctx);

            uv_buf_t uvbuf = { internal::rte_context::buflen ,ctx->buf };
			error err = uv_fs_read(uv_default_loop(), req, fd, &uvbuf, 1, 0, internal::rte_cb<decltype(callback)>);
            if(err) 
            {
                // failed to initiate uv_fs_read()
                internal::delete_req<decltype(callback), internal::rte_context>(req);
            }
            return err;
        }

        inline error close(file_handle fd, std::function<void(error e)>&& callback)
        {
            auto req = internal::create_req(std::move(callback));
			error err = uv_fs_close(uv_default_loop(), req, fd, [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_CLOSE);
                internal::invoke_from_req<decltype(callback)>(req, req->result);
                internal::delete_req(req);
            });
            if (err)
            {
                internal::delete_req(req);
            }
            return err;
        }

        inline error unlink(const std::string& path, std::function<void(error e)>&& callback)
        {
            auto req = internal::create_req(std::move(callback));
			error err = uv_fs_unlink(uv_default_loop(), req, path.c_str(), [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_UNLINK);
                internal::invoke_from_req<decltype(callback)>(req, req->result);
                internal::delete_req(req);
            });
            if (err) 
            {
                internal::delete_req(req);
            }
            return err;
        }

        inline error mkdir(const std::string& path, int mode, std::function<void(error e)>&& callback)
        {
            auto req = internal::create_req(std::move(callback));
			error err = uv_fs_mkdir(uv_default_loop(), req, path.c_str(), mode, [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_MKDIR);
                internal::invoke_from_req<decltype(callback)>(req, req->result);
                internal::delete_req(req);
            }); 
            if (err) 
            {
                internal::delete_req(req);
            }
            return err;
        }

        inline error rmdir(const std::string& path, std::function<void(error e)>&& callback)
        {
            auto req = internal::create_req(std::move(callback));
			error err = uv_fs_rmdir(uv_default_loop(), req, path.c_str(), [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_RMDIR);
                internal::invoke_from_req<decltype(callback)>(req, req->result);
                internal::delete_req(req);
            });
            if (err)
            {
                internal::delete_req(req);
            }
            return err;
        }

        inline error rename(const std::string& path, const std::string& new_path, std::function<void(error e)>&& callback)
        {
            auto req = internal::create_req(std::move(callback));
			error err = uv_fs_rename(uv_default_loop(), req, path.c_str(), new_path.c_str(), [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_RENAME);
                internal::invoke_from_req<decltype(callback)>(req, req->result);
                internal::delete_req(req);
            });
            if (err)
            {
                internal::delete_req(req);
            }
            return err;
        }

        inline error chmod(const std::string& path, int mode, std::function<void(error e)>&& callback)
        {
            auto req = internal::create_req(std::move(callback));
			error err = uv_fs_chmod(uv_default_loop(), req, path.c_str(), mode, [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_CHMOD);
                internal::invoke_from_req<decltype(callback)>(req, req->result);
                internal::delete_req(req);
            });
            if (err)
            {
                internal::delete_req(req);
            }
            return err;
        }

        inline error chown(const std::string& path, int uid, int gid, std::function<void(error e)>&& callback)
        {
            auto req = internal::create_req(std::move(callback));
            error err = uv_fs_chown(uv_default_loop(), req, path.c_str(), uid, gid, [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_CHOWN);
                internal::invoke_from_req<decltype(callback)>(req, req->result);
                internal::delete_req(req);
            });
            if (err)
            {
                internal::delete_req(req);
            }
            return err;
        }

#if 0
		error readdir(const std::string& path, int flags, std::function<void(error e)>&& callback)
        {
            auto req = internal::create_req(std::move(callback));
			error err = uv_fs_readdir(uv_default_loop(), req, path.c_str(), flags, [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_READDIR);
                internal::invoke_from_req<decltype(callback)>(req, req->result);
                internal::delete_req(req);
            });
            if (err)
            {
                internal::delete_req(req);
            }
            return err;
        }

		error stat(const std::string& path, std::function<void(error e)>&& callback)
        {
            auto req = internal::create_req(std::move(callback));
			error err = uv_fs_stat(uv_default_loop(), req, path.c_str(), [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_STAT);
                internal::invoke_from_req<decltype(callback)>(req, req->result);
                internal::delete_req(req);
            }); 
            if (err)
            {
                internal::delete_req(req);
            }
            return err;
        }

		error fstat(const std::string& path, std::function<void(error e)>&& callback)
        {
            auto req = internal::create_req(std::move(callback));
			error err = uv_fs_fstat(uv_default_loop(), req, path.c_str(), [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_FSTAT);
                internal::invoke_from_req<decltype(callback)>(req, req->result);
                internal::delete_req(req);
            });
            if (err)
            {
                internal::delete_req(req);
            }
            return err;
        }
#endif
    }

    class file
    {
    public:
        static bool read(const std::string& path, std::function<void(const std::string& str, error e)>&& callback)
        {
            return 0 == fs::open(path.c_str(), fs::read_only, 0, [=,cb = std::move(callback)](fs::file_handle fd, error e)mutable {
                if(e)
                {
                    callback(std::string(), e);
                }
                else
                {
					error err = fs::read_to_end(fd, std::move(cb));
                    if (err)
                    {
                        // failed to initiate read_to_end()
                        callback(std::string(), error(err));
                    }
                }
            });
        }

        static bool write(const std::string& path, const std::string& str, std::function<void(int nwritten, error e)>&& callback)
        {
            return 0 == fs::open(path.c_str(), fs::write_only|fs::create, 0664, [=, cb = std::move(callback)](fs::file_handle fd, error e)mutable {
                if(e)
                {
                    callback(0, e);
                }
                else
                {
					error err = fs::write(fd, str.c_str(), str.length(), 0, std::move(cb));
                    if (err)
                    {
                        // failed to initiate read_to_end()
                        callback(0, error(err));
                    }
                }
            });
        }
    };
}

#endif
