#ifndef _THREAD_POOL_H
#define _THREAD_POOL_H
#include <iostream>
#include <list>
#include <pthread.h>
#include <exception>
#include <string>
template <typename type>
class Thread_Pool
{
private:
    static void *worker(void *argument);
    void run();

private:
    int current_thread_number;
    int max_client_requests;
    pthread_t *thread_array_pt;
    std::list<type *> client_request_quene;
    bool thread_pool_stop;
    static pthread_cond_t thread_condition;
    static pthread_mutex_t thread_exclude;

public:
    Thread_Pool(int current_thread_number = 8, int max_client_requests = 60000);
    bool append_to_list(type *client_request);
    ~Thread_Pool();
};

class error_information : public std::exception
{
public:
    std::string error_string;

public:
    error_information(std::string &reference_error_information);
    const char *what() const throw();
    ~error_information();
};

error_information::error_information(std::string &reference_error_information) : error_string(reference_error_information)
{
}

const char *error_information::what() const throw()
{
    return error_string.c_str();
}

error_information::~error_information(){};

template<typename type>
pthread_cond_t Thread_Pool<type>::thread_condition = PTHREAD_COND_INITIALIZER;

template<typename type>
pthread_mutex_t Thread_Pool<type>::thread_exclude = PTHREAD_MUTEX_INITIALIZER;

template <typename type>
Thread_Pool<type>::Thread_Pool(int temp_current_thread_number, int temp_max_client_requests) : current_thread_number(temp_current_thread_number), max_client_requests(temp_current_thread_number),
thread_array_pt(NULL), thread_pool_stop(true)
{
    if (current_thread_number <= 0 || max_client_requests <= 0)
    {
        std::string error_string="Initialize data error.\n";
        throw error_information(error_string);
    }
        
    thread_array_pt = new pthread_t[current_thread_number];
    if (!thread_array_pt)
    {
        std::string error_string="New function error.\n";
        throw error_information(error_string);
    }
    for (size_t i = 0; i < current_thread_number; i++)
    {
        if (pthread_create(&thread_array_pt[i], NULL, &worker, this) != 0)
        {
            delete[] thread_array_pt;
            std::string error_string="Pthread_create error.\n";
            throw error_information(error_string);
            /* code */
        }
        /*if (pthread_detach(thread_array_pt[i]))
        {
            delete[] thread_array_pt;
            std::string error_string="Pthread_detach error.\n";
            throw error_information(error_string);
        }*/
        /* code */
    }
}
template <typename type>
bool Thread_Pool<type>::append_to_list(type *client_request)
{
    pthread_mutex_lock(&thread_exclude);
    if (client_request_quene.size() > max_client_requests)
    {
        pthread_mutex_unlock(&thread_exclude);
        return false;
    }
    client_request_quene.push_back(client_request);
    pthread_mutex_unlock(&thread_exclude);
    pthread_cond_signal(&thread_condition);
    return true;
}
template <typename type>
void Thread_Pool<type>::run()
{
    //std::cout<<"Run function"<<std::endl;
    while (thread_pool_stop)
    {
        pthread_mutex_lock(&thread_exclude);
        while (client_request_quene.empty())
        {
            pthread_cond_wait(&thread_condition, &thread_exclude);
            /* code */
        }
        type *client_request = client_request_quene.front();
        client_request_quene.pop_front();
        pthread_mutex_unlock(&thread_exclude);
        if (!client_request)
        {
            continue;
        }
        else
        {
            //std::cout<<"Pthread ID: "<<pthread_self()<<std::endl;
            client_request->process();
        }
        /* code */
    }
}
template <typename type>
void *Thread_Pool<type>::worker(void *argument)
{
    Thread_Pool<type> *thread_pool_pt = (Thread_Pool<type> *)argument;
    //std::cout<<(int *)thread_pool_pt<<std::endl;
    //std::cout<<"Pthread ID: "<<pthread_self()<<std::endl;
    thread_pool_pt->run();
    return thread_pool_pt;
}
template <typename type>
Thread_Pool<type>::~Thread_Pool()
{
    int i=0;
    thread_pool_stop = false;
    while (current_thread_number>0)
    {
        pthread_join(thread_array_pt[i],NULL);
        i++;
        current_thread_number--;
    }

    delete[] thread_array_pt;
    
}

#endif