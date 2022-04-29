#include "http_connect.h"
#include <signal.h>
#include <errno.h>
#include <unistd.h> //debug
#include <sys/un.h> //debug
//#include <string>
#define _POSIX_C_SOURCE 200809L
#define MAX_CLIENT_SERVER 60000
#define MAX_EPOLL_EVENT_COUNT 60000
extern void add_epoll_fd(int epollfd, int fd, bool oneshot);
extern void remove_epoll_fd(int epollfd, int fd);

void signal_handler(int)
{
}

void add_signal(int signal_number, void(handler)(int), bool restart = true)
{
    int function_flag = 0;
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    if (restart)
    {
        sa.sa_flags |= SA_RESTART;
        /* code */
    }
    sa.sa_handler = handler;
    sigfillset(&sa.sa_mask);
    function_flag = sigaction(signal_number, &sa, NULL);
    if (function_flag < 0)
    {
        std::string error_string = "Sigaction function error\n";
        throw error_information(error_string);
    }
}

/*int debug_socket()
{
    int function_flag;
    char debug_socket_absolute_name[108];
    memset(debug_socket_absolute_name, '\0', sizeof(debug_socket_absolute_name));
    strncpy(debug_socket_absolute_name, "/var/tmp/www/web_client/test/debug_file", strlen("/var/tmp/www/web_client/test/debug_file") + 1);

    function_flag = unlink(debug_socket_absolute_name);

    if (function_flag < 0 && errno != ENOENT)
    {
        std::cout << "Position 47." << std::endl;
        std::string error_string = strerror(errno);
        throw error_information(error_string);
    }

    std::cout << "Remove :" << debug_socket_absolute_name << std::endl;

    int socket_debug = socket(AF_UNIX, SOCK_STREAM, 0);

    if (socket_debug < 0)
    {
        std::cout << "Position 57." << std::endl;
        std::string error_string = strerror(errno);
        throw error_information(error_string);
    }

    struct sockaddr_un address_debug;
    memset(&address_debug, '\0', sizeof(address_debug));
    address_debug.sun_family = AF_UNIX;
    strncpy(address_debug.sun_path, debug_socket_absolute_name, strlen(debug_socket_absolute_name) + 1);

    function_flag = bind(socket_debug, (struct sockaddr *)&address_debug, sizeof(address_debug));

    if (function_flag < 0)
    {
        std::cout << "Position 71." << std::endl;
        std::string error_string = strerror(errno);
        throw error_information(error_string);
    }

    function_flag = listen(socket_debug, 5);

    if (function_flag < 0)
    {
        std::cout << "Position 79." << std::endl;
        std::string error_string = strerror(errno);
        throw error_information(error_string);
    }

    return socket_debug;
}*/

int socket_listen_function(const char *ip_address, int port_number)
{
    int socket_listen;

    socket_listen = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_listen < 0)
    {
        std::string error_string = strerror(errno);
        throw error_information(error_string);
    }

    struct linger linger_option;
    linger_option.l_onoff = 1;
    linger_option.l_linger = 0;
    setsockopt(socket_listen, SOL_SOCKET, SO_LINGER, &linger_option, sizeof(linger_option));
    int function_flag = 0;
    struct sockaddr_in address_server;

    memset(&address_server, '\0', sizeof(address_server));

    address_server.sin_family = AF_INET;
    inet_pton(AF_INET, ip_address, &address_server.sin_addr);
    address_server.sin_port = htons(port_number);
    function_flag = bind(socket_listen, (struct sockaddr *)&address_server, sizeof(address_server));
    if (function_flag < 0)
    {
        std::string error_string = strerror(errno);
        throw error_information(error_string);
    }
    function_flag = listen(socket_listen, 5);

    if (function_flag < 0)
    {
        std::string error_string;
        throw error_information(error_string);
    }

    setnonblocking(socket_listen);
    return socket_listen;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::string error_string = "Usage: software_name ip_address port.\n";
        throw error_information(error_string);
    }

    const char *ip_address = argv[1];
    int port_number = atoi(argv[2]);

    add_signal(SIGPIPE, SIG_IGN); // Ignore SIGPIPE signal.
    Thread_Pool<Http_Connect> *thread_pool_pt = NULL;

    try
    {
        thread_pool_pt = new Thread_Pool<Http_Connect>; // Create thread pool.
    }
    catch (...)
    {
        std::string error_string = "Create thread poll error.\n";
        throw error_information(error_string);
    }

    Http_Connect *client_pt = new Http_Connect[MAX_CLIENT_SERVER]; // Create Http_Connect object.
    if (!client_pt)
    {
        std::string error_string = "New Http_Connect memory error.\n";
        throw error_information(error_string);
        /* code */
    }

    int client_server_count = 0;
    // int socket_listen = debug_socket(); // debug function

    int socket_listen = socket_listen_function(ip_address, port_number);

    struct epoll_event epoll_ready_events[MAX_EPOLL_EVENT_COUNT];
    int thread_epoll_fd = epoll_create(5);
    if (thread_epoll_fd < 0)
    {
        std::string error_string = strerror(errno);
        throw error_information(error_string);
        /* code */
    }
    add_epoll_fd(thread_epoll_fd, socket_listen, false);
    Http_Connect::hc_thread_epollfd = thread_epoll_fd;
    while (true)
    {
        int epoll_ready_number = epoll_wait(thread_epoll_fd, epoll_ready_events, MAX_EPOLL_EVENT_COUNT, -1);
        if (epoll_ready_number < 0 && errno != EINTR)
        {
            std::string error_string = strerror(errno);
            throw error_information(error_string);
            /* code */
        }
        for (size_t i = 0; i < epoll_ready_number; i++)
        {
            int socket_ready = epoll_ready_events[i].data.fd;
            if (socket_ready == socket_listen) // Socket_listen ready;
            {
                struct sockaddr_in address_client;
                memset(&address_client, '\0', sizeof(address_client));
                socklen_t address_client_length = sizeof(address_client);
                int socket_client = accept(socket_listen, (sockaddr *)&address_client, &address_client_length);
                if (socket_client < 0)
                {
                    std::string errro_string = strerror(errno);
                    throw error_information(errro_string);
                    /* code */
                }
                if (Http_Connect::hc_client_count >= MAX_CLIENT_SERVER) // If client count more than MAX_CLIENT_SERVER, give up the client connection and send associated information to client.
                {
                    std::string error_string = "Server busy.\n";
                    send(socket_client, error_string.c_str(), sizeof(error_string), 0);
                    close(socket_client);
                    continue;
                    /* code */
                }
                client_pt[socket_client].initialize_new_connect(socket_client, &address_client);

                /* code */
            }
            else if (epoll_ready_events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
            {
                client_pt[socket_ready].close_connet(true);
                /* code */
            }
            else if (epoll_ready_events[i].events & EPOLLIN)
            {
                //std::cout << "socket_ready" << socket_ready << std::endl;
                if (client_pt[socket_ready].read_from_client_socket())
                {

                    thread_pool_pt->append_to_list(&client_pt[socket_ready]);
                    // client_pt[socket_ready].process();
                }
                else
                {
                    client_pt[socket_ready].close_connet(true);
                }

                /* code */
            }
            else if (epoll_ready_events[i].events & EPOLLOUT)
            {
                //std::cout << "socket_ready" << socket_ready << std::endl;
                if (client_pt[socket_ready].write_respond_to_socket())
                {
                   // std::cout << "ok" << std::endl;
                }
                else
                {
                    client_pt[socket_ready].close_connet(true);
                }
            }
            else
            {
            }
            /* code */
        }
        /* code */
    }
    close(socket_listen);
    close(thread_epoll_fd);
    delete[] client_pt;
    delete thread_pool_pt;
    return 0;
}
