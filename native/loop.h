#ifndef __LOOP_H__
#define __LOOP_H__

#include "base.h"
#include "error.h"

namespace native
{
    /*!
     *  Class that represents the loop instance.
     */
    class loop
    {
    public:
        /*!
         *  Default constructor
         *  @param use_default indicates whether to use default loop or create a new loop.
         */
        loop(bool use_default=true)
            : uv_loop_(use_default ? uv_default_loop() : uv_loop_new())
        { }

        /*!
         *  Destructor
         */
        ~loop()
        {
            if(uv_loop_)
            {
                uv_loop_delete(uv_loop_);
                uv_loop_ = nullptr;
            }
        }

        /*!
         *  Returns internal handle for libuv functions.
         */
        uv_loop_t* get() { return uv_loop_; }

        /*!
         *  Starts the loop.
         *  Internally, this function just calls uv_run() function.
         */
        error run() { 

            /* new libuv stuff */
            return uv_run(uv_loop_, UV_RUN_DEFAULT); 
        }

        /*!
         *  Polls for new events without blocking.
         *  Internally, this function just calls uv_run_once() function.
         */
		error run_once() {

            //return uv_run_once(uv_loop_)==0; 
     
            return uv_run(uv_loop_, UV_RUN_ONCE); 
        }

        /*!
         *  ...
         *  Internally, this function just calls uv_update_time() function.
         */
        void update_time() { uv_update_time(uv_loop_); }

        /*!
         *  ...
         *  Internally, this function just calls uv_now() function.
         */
        int64_t now() { return uv_now(uv_loop_); }

        /*!
         *  Returns the last error occured in the loop.
         */
    private:
        loop(const loop&) = delete;
        void operator =(const loop&) =delete;

    private:
        uv_loop_t* uv_loop_;
    };

    /*!
     *  Starts the default loop.
     */
    inline error run()
    {
        /*New libuv requires a runmode enum argument*/
        return uv_run(uv_default_loop(),UV_RUN_DEFAULT);
    }

    /*!
     *  Polls for new events without blocking for the default loop.
     */
    inline error run_once()
    {
        /*New libuv requires a runmode argument*/
        return uv_run(uv_default_loop(),UV_RUN_ONCE);
    }
}


#endif