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

		template<typename T>
		struct us_fs_task_t : public uv_fs_t, public promise_data_base_t<T>
		{
			using type = T;
		};


        task<std::tuple<native::fs::file_handle, error>> open(const std::string& path, int flags, int mode)
        {
			PROMISE_REQ_HEAD(us_fs_task_t, std::tuple<native::fs::file_handle, error>);

			error err = uv_fs_open(uv_default_loop(), &uv_req, path.c_str(), flags, mode, [](uv_fs_t* req) {
                assert(req->fs_type == UV_FS_OPEN);
				auto val = static_cast<promise_cur_data_t*>(req);
				val->set_value(data_type(req->result<0 ? file_handle(-1) : req->result,req->result));
				val->resume();
            });

			co_await ret;
			return std::move(ret_value);
		}

		task<std::tuple<std::string, error>> read(file_handle fd, size_t len, off_t offset)
        {
			PROMISE_REQ_HEAD(us_fs_task_t, std::tuple<std::string, error>);
			 
			std::get<0>(ret_value).resize(len);
			uv_buf_t uvbuf = {len, const_cast<char*>(std::get<0>(ret_value).c_str()) };

			error err = uv_fs_read(uv_default_loop(), &uv_req, fd, &uvbuf, 1, offset, [](uv_fs_t* req) {
                assert(req->fs_type == UV_FS_READ);
				auto val = static_cast<promise_cur_data_t*>(req);
				if (req->result < 0)
                {
                    // system error
					val->set_value(data_type(std::string() , req->result));
                }
                else if(req->result == 0)
                {
                    // EOF
					val->set_value(data_type(std::string(), UV_EOF));
                }
                else
                {
					std::get<0>(val->value_).resize(req->result);
                }
				val->resume();
            });

			co_await ret;
			return ret_value;
		}

		task<std::tuple<std::string, error>> read_to_end(file_handle fd)
        {
			PROMISE_REQ_HEAD(us_fs_task_t, std::tuple<std::string, error>);
			
			auto &val = std::get<0>(ret_value);
			const size_t block_size = 4096;
			long offset = 0;
			std::vector<std::string> collection;
			do
			{
				std::tuple<std::string, error> block = co_await read(fd, block_size, 0);
				error err = std::get<1>(block);
				if (err.code() > 0) //more to read
				{
					collection.emplace_back(std::move(std::get<0>(block)));
				}
				else if(err.code() == 0) //no more data
				{
					collection.emplace_back(std::move(std::get<0>(block)));
					size_t total = 0;
					for (auto&a : collection)
						total += a.size();
					val.resize(total);
					for (auto&a : collection)
						val.append(a);
					break;
				}
				else //error
				{
					std::get<1>(ret_value) = std::get<1>(block);
					break;
				}
				offset += block_size;
			} while (1);
			
			return ret_value;
		}

		task<std::tuple<int, error>> write(file_handle fd, const char* buf, size_t len, off_t offset)
		{
			PROMISE_REQ_HEAD(us_fs_task_t, std::tuple<int, error>);

			uv_buf_t uvbuf = { len, const_cast<char*>(buf) };
			// TODO: const_cast<> !!
			error err = uv_fs_write(uv_default_loop(), &uv_req, fd, &uvbuf, 1, offset, [](uv_fs_t* req) {
				assert(req->fs_type == UV_FS_WRITE);
				auto val = static_cast<promise_cur_data_t*>(req);
				if (req->result < 0)
				{
					val->set_value(data_type(0, req->result));
				}
				else
				{
					val->set_value(data_type(req->result, 0));
				}
				val->resume();
			});
			co_await ret;
			return ret_value;
		}

        task<error> close(file_handle fd)
        {
			PROMISE_REQ_HEAD(us_fs_task_t, error);
			error err = uv_fs_close(uv_default_loop(), &uv_req, fd, [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_CLOSE);
				auto val = static_cast<promise_cur_data_t*>(req);
				val->set_value(req->result);
				val->resume();
            });

			co_await ret;
			return ret_value;
        }

		task<error> unlink(const std::string& path)
        {
			PROMISE_REQ_HEAD(us_fs_task_t, error);

			error err = uv_fs_unlink(uv_default_loop(), &uv_req, path.c_str(), [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_UNLINK);
				auto val = static_cast<promise_cur_data_t*>(req);
				val->set_value(req->result);
				val->resume();
			});
			co_await ret;
			return ret_value;
		}

		task<error> mkdir(const std::string& path, int mode)
        {
			PROMISE_REQ_HEAD(us_fs_task_t, error);

			error err = uv_fs_mkdir(uv_default_loop(), &uv_req, path.c_str(), mode, [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_MKDIR);
				auto val = static_cast<promise_cur_data_t*>(req);
				val->set_value(req->result);
				val->resume();
			});
			co_await ret;
			return ret_value;
		}

		task<error> rmdir(const std::string& path)
        {
			PROMISE_REQ_HEAD(us_fs_task_t, error);
			error err = uv_fs_rmdir(uv_default_loop(), &uv_req, path.c_str(), [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_RMDIR);
				auto val = static_cast<promise_cur_data_t*>(req);
				val->set_value(req->result);
				val->resume();
			});
			co_await ret;
			return ret_value;
		}

		task<error> rename(const std::string& path, const std::string& new_path)
        {
			PROMISE_REQ_HEAD(us_fs_task_t, error);
			error err = uv_fs_rename(uv_default_loop(), &uv_req, path.c_str(), new_path.c_str(), [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_RENAME);
				auto val = static_cast<promise_cur_data_t*>(req);
				val->set_value(req->result);
				val->resume();
			});
			co_await ret;
			return ret_value;
		}

		task<error> chmod(const std::string& path, int mode)
        {
			PROMISE_REQ_HEAD(us_fs_task_t, error);
			error err = uv_fs_chmod(uv_default_loop(), &uv_req, path.c_str(), mode, [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_CHMOD);
				auto val = static_cast<promise_cur_data_t*>(req);
				val->set_value(req->result);
				val->resume();
			});
			co_await ret;
			return ret_value;
		}

        task<error> chown(const std::string& path, int uid, int gid)
        {
			PROMISE_REQ_HEAD(us_fs_task_t, error);
			error err = uv_fs_chown(uv_default_loop(), &uv_req, path.c_str(), uid, gid, [](uv_fs_t* req) {
                assert(req->fs_type == UV_FS_CHOWN);
				auto val = static_cast<promise_cur_data_t*>(req);
				val->set_value(req->result);
				val->resume();
			});

			co_await ret;
			return ret_value;
		}

#if 0
		error readdir(const std::string& path, int flags, std::function<void(error e)>&& callback)
        {
			PROMISE_REQ_HEAD(us_fs_task_t, error);
			error err = uv_fs_readdir(uv_default_loop(), &uv_req, path.c_str(), flags, [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_READDIR);
				auto val = static_cast<promise_cur_data_t*>(req);
				val->set_value(req->result);
				val->resume();
			});
			co_await ret;
			return ret_value;
		}

		error stat(const std::string& path, std::function<void(error e)>&& callback)
        {
			PROMISE_REQ_HEAD(us_fs_task_t, error);
			error err = uv_fs_stat(uv_default_loop(), &uv_req, path.c_str(), [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_STAT);
				auto val = static_cast<promise_cur_data_t*>(req);
				val->set_value(req->result);
				val->resume();
			});
			co_await ret;
			return ret_value;
		}

		error fstat(const std::string& path, std::function<void(error e)>&& callback)
        {
			PROMISE_REQ_HEAD(us_fs_task_t, error);
			error err = uv_fs_fstat(uv_default_loop(), &uv_req, path.c_str(), [](uv_fs_t* req){
                assert(req->fs_type == UV_FS_FSTAT);
				auto val = static_cast<promise_cur_data_t*>(req);
				val->set_value(req->result);
				val->resume();
			});
			co_await ret;
			return ret_value;
		}
#endif
    }

    class file
    {
    public:
        static task<std::tuple<std::string,error>> read(const std::string& path)
        {
			std::tuple<std::string, error> ret;
			auto result = co_await fs::open(path.c_str(), fs::read_only, 0);
			if (std::get<0>(result) == -1)
			{
				std::get<1>(ret) = std::get<1>(result);
			}
			else
			{
				ret = co_await fs::read_to_end(std::get<0>(result));
			}
			return ret;
        }

        static task<std::tuple<native::fs::file_handle, error>> write(const std::string& path, const std::string& str)
        {
			std::tuple<native::fs::file_handle, error> ret;
			auto result = co_await fs::open(path.c_str(), fs::read_only, 0);
			if (std::get<0>(result) == -1)
			{
				std::get<1>(ret) = std::get<1>(result);
			}
			else
			{
				ret = co_await fs::write(std::get<0>(result), str.c_str(), str.length(), 0);
			}
			return ret;
        }
    };
}

#endif
