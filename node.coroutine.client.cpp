// task.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "../include/coroutine_tasks.hpp"

int data = 42;
#include <iostream>
int main() {
    using fn = int(*)();
    fn func = []()->int {
        std::cout << ++data << " in func \n";
        return data;
    };
    fn fund = []()->int {
        std::cout << ++data << " in fund \n";
        return data;
    };
    auto func2 = [&]()->int {
        std::cout << ++data << " in func2 \n";
        return data;
    };
    //empty task then
    {
        auto old_task = coroutine_tasks::make_task();
        auto old_task_handle = old_task.get_promise_handle();
        auto new_task = old_task.then(func);
        old_task_handle.resume();
        auto v = new_task.cur_value_ref();
    }
    //make_task then and then
    {
        auto old_task = coroutine_tasks::make_task(func);
        auto old_task_handle = old_task.get_promise_handle();
        auto new_task = old_task.then(fund);
        old_task_handle.resume();
        auto v = new_task.cur_value_ref();
    }
    {
        auto old_task = coroutine_tasks::make_task(
        []()->short {
            std::cout << "start";
            return 33; });
        auto old_task_handle = old_task.get_promise_handle();
        auto new_task = old_task.then(func2)
        .then([](int &v)->short {
            std::cout << "get" << v;
            return 33; })
        .then([](short & v)->coroutine_tasks::task<int> {
            co_await coroutine_tasks::ex::suspend_never{};
            return v;
        });
        old_task_handle.resume();
        auto v = new_task.cur_value_ref();
    }
    //then task
    {
        auto old_task = coroutine_tasks::make_task(func);
        auto old_task_handle = old_task.get_promise_handle();
        auto new_task = old_task.then([]() ->coroutine_tasks::task<int> {
            co_await coroutine_tasks::ex::suspend_never{};
            printf("haha");
            return 3;
        });
        old_task_handle.resume();
        auto v = new_task.cur_value_ref();
    }
    //when_all zip
    {
        auto task_a = coroutine_tasks::make_task(func);
        auto task_b = coroutine_tasks::make_task(func);
        auto task_a_handle = task_a.get_promise_handle();
        auto task_b_handle = task_b.get_promise_handle();
        auto new_task = coroutine_tasks::when_all(task_a, task_b).then([](std::tuple<int, int> &) {printf("ok"); });
        task_a_handle.resume();
        task_b_handle.resume();
        auto v = new_task.cur_value_ref();
    }
    //when_all map
    {
        std::vector<coroutine_tasks::task<int>> tasks;
        tasks.emplace_back(coroutine_tasks::make_task(func));
        tasks.emplace_back(coroutine_tasks::make_task(func));
        auto task_handle_a = tasks[0].get_promise_handle();
        auto task_handle_b = tasks[1].get_promise_handle();
        auto new_task = coroutine_tasks::when_all(tasks.begin(),
        tasks.end()).then([](std::vector<int> &) {printf("ok"); });
        task_handle_a.resume();
        task_handle_b.resume();
        auto v = new_task.cur_value_ref();
    }
    //when_n all
    {
        std::vector<coroutine_tasks::task<int>> tasks;
        tasks.emplace_back(coroutine_tasks::make_task(func));
        tasks.emplace_back(coroutine_tasks::make_task(func));
        tasks.emplace_back(coroutine_tasks::make_task(func));
        auto task_handle_a = tasks[0].get_promise_handle();
        auto task_handle_b = tasks[1].get_promise_handle();
        auto task_handle_c = tasks[2].get_promise_handle();
        auto new_task = coroutine_tasks::when_n(tasks.begin(), tasks.end(), tasks.size())
        .then([](std::vector<std::pair<size_t, int>> &) {
            printf("ok");
        });
        task_handle_a.resume();
        task_handle_b.resume();
        task_handle_c.resume();
        auto v = new_task.cur_value_ref();
    }
    //when_n some
    {
        std::vector<coroutine_tasks::task<int>> tasks;
        tasks.emplace_back(coroutine_tasks::make_task(func));
        tasks.emplace_back(coroutine_tasks::make_task(func));
        tasks.emplace_back(coroutine_tasks::make_task(func));
        auto task_handle_a = tasks[0].get_promise_handle();
        auto task_handle_b = tasks[1].get_promise_handle();
        auto task_handle_c = tasks[2].get_promise_handle();
        auto new_task = coroutine_tasks::when_n(tasks.begin(), tasks.end(), 2)
        .then([](std::vector<std::pair<size_t, int>> &) {
            printf("ok");
        });
        task_handle_a.resume();
        task_handle_b.resume();
        //task_handle_c.resume();
        auto v = new_task.cur_value_ref();
    }
    //when_n one
    {
        std::vector<coroutine_tasks::task<int>> tasks;
        tasks.emplace_back(coroutine_tasks::make_task(func));
        tasks.emplace_back(coroutine_tasks::make_task(func));
        tasks.emplace_back(coroutine_tasks::make_task(func));
        auto task_handle_a = tasks[0].get_promise_handle();
        auto task_handle_b = tasks[1].get_promise_handle();
        auto task_handle_c = tasks[2].get_promise_handle();
        auto new_task = coroutine_tasks::when_n(tasks.begin(), tasks.end(), 1)
        .then([](std::vector<std::pair<size_t, int>> &xx) {
            printf("ok");
        })
        .then([]() {
            printf("ok");
        });
        task_handle_a.resume();
        //task_handle_b.resume();
        //task_handle_c.resume();
        auto v = new_task.cur_value_ref();
    }
    //when_any
    {
        std::vector<coroutine_tasks::task<int>> tasks;
        tasks.emplace_back(coroutine_tasks::make_task(func));
        tasks.emplace_back(coroutine_tasks::make_task(func));
        tasks.emplace_back(coroutine_tasks::make_task(func));
        auto task_handle_a = tasks[0].get_promise_handle();
        auto task_handle_b = tasks[1].get_promise_handle();
        auto task_handle_c = tasks[2].get_promise_handle();
        auto new_task = coroutine_tasks::when_any(tasks.begin(), tasks.end())
        .then([](std::pair<size_t, int> &xx) {
            printf("ok");
        });
        task_handle_a.resume();
        //task_handle_b.resume();
        //task_handle_c.resume();
        auto v = new_task.cur_value_ref();
    }
    //when_any
    {
        std::vector<coroutine_tasks::task<int>> tasks;
        tasks.emplace_back(coroutine_tasks::make_task(func));
        tasks.emplace_back(coroutine_tasks::make_task(func));
        tasks.emplace_back(coroutine_tasks::make_task(func));
        auto task_handle_a = tasks[0].get_promise_handle();
        auto task_handle_b = tasks[1].get_promise_handle();
        auto task_handle_c = tasks[2].get_promise_handle();
        auto new_task = coroutine_tasks::when_any(tasks.begin(), tasks.end())
        .then([](std::pair<size_t, int> &xx) ->coroutine_tasks::task<std::pair<size_t, int>> {
            co_await coroutine_tasks::ex::suspend_never{};
            printf("ok");
            return xx;
        });
        task_handle_a.resume();
        //task_handle_b.resume();
        //task_handle_c.resume();
        auto v = new_task.cur_value_ref();
    }
    return 0;
}
