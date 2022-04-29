#ifndef _HTTP_CONNECT_H
#define _HTTP_CONNECT_H
#include <iostream>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <sys/uio.h>
#include "thread_pool.h"
class Http_Connect
{
public:
    static const int MAX_FILE_PATH = 256;
    static const int MAX_READ_BUFFER = 2048;
    static const int MAX_WRITE_BUFFER = 1024;
    enum HTTP_METHOD
    {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE
    }; // The http method, this software just implement GET method;
    enum MAJOR_THREAD_STATE
    {
        THREAD_STATE_ANALYSIS_REQUEST_LINE = 0,
        THREAD_STATE_ANALYSIS_HEADER,
        THREAD_STATE_ANALYSIS_CONTENT
    }; // THe current state of major thread, when the major thread analysis client request.
    enum SERVER_ANALYSIS_RESULT
    {
        REQUEST_INCOMPLETE = 0,
        REQUEST_INTEGRITY,
        REQUEST_ERROR,
        REQUSET_TEST,
        SOURCE_NOT_FIND,
        REQUEST_NO_PERMISSION,
        REQUEST_GET_FILE,
        ERROR_INTERNAL,
        CONNECT_CLOSE
    }; // The result of server analysis http request.
    enum READ_LINE_STATE
    {
        LINE_INTEGRITY = 0,
        LINE_BAD,
        LINE_INCOMPLETE
    };
    static int hc_thread_epollfd;
    static int hc_client_count;

private:
    MAJOR_THREAD_STATE hc_thread_check_state;
    HTTP_METHOD hc_method;
    int hc_client_socket;                        // Client connect socket.
    struct sockaddr_in hc_address_client;        // Client connect ip address;
    char hc_read_buffer[MAX_READ_BUFFER];        // A buffer of thread store client request.
    int hc_current_read_buffer_next_empty_index; // An index of hc_read_buffer point current hc_read_buffer next empty size.
    int hc_current_analysis_char_index;          // An index of hc_read_buffer point current analysis char.
    int hc_current_analysis_line_start_index;    // An index of hc_read_buffer point a new line start, the new line will be analysis by function.
    char hc_write_buffer[MAX_WRITE_BUFFER];      // A buffer of thread store information that ready to send client.
    int hc_write_buffer_ready_to_send_count;     // The hc_write_buffer has stored size count.
    int hc_respond_have_send_count;              // The byte count of server have send to client.
    int hc_respond_ready_to_send_count;          // The byte count of server ready send to client(respond byte size and file byte size);
    char hc_absolute_file_path[MAX_FILE_PATH];   // The client request file absolutely path.
    char *hc_file_pathname;                      // The file relative path, reference path: website root(/var/www/test).
    char *hc_http_version;
    char *hc_server_name;          // Server name or host name.
    int hc_request_content_length; // Client request entity length.
    bool hc_linger;                // Socket option LINGER.
    bool test_server;
    char *hc_map_file_address;   // Client request file map address.
    struct stat hc_file_state;   // File state.
    struct iovec hc_iovector[2]; // Writev function array buffer.
    int hc_iovec_memory_number;  // Write function buffer count.

private:
    void initialize();
    READ_LINE_STATE parse_line();
    char *get_read_buffer_pt();
    /*{
        return hc_read_buffer + hc_current_analysis_line_start_index;
    }*/
    SERVER_ANALYSIS_RESULT analysis_http_request(); // Major finite state machine.
    // They are invoked by analysis_http_request function to process http request.

    SERVER_ANALYSIS_RESULT parse_request_line(char *http_request_text); // Minor finite state machine.
    SERVER_ANALYSIS_RESULT parse_headers(char *http_request_text);      // Minor finite state machine.
    SERVER_ANALYSIS_RESULT parse_content(char *http_request_text);      // Minor finite state machine.
    SERVER_ANALYSIS_RESULT execute_request();                           // When we have a whole client http request, execute the request.
    void unmap();

    bool write_http_respond(SERVER_ANALYSIS_RESULT result); // According the result of function process client request, we decide to write appropriate respond information to hc_write_buffer.
    // They are invoked by write_http_respond function to process http respond.

    bool add_server_respond(int number, ...); // Major function is use to write respond information to hc_write_buffer.

    bool add_respond_line(int state_code, const char *state_title); // Add respond line.

    bool add_content_length(int content_length); // File content length.
    bool add_linger();                           // Whether keep connect.
    bool add_blank_line();

    bool add_headers(int content_length); // Add respond headers.

    bool add_content(const char *content); // Add client request file content.
public:
    Http_Connect(/* args */); // Constructor
    ~Http_Connect();

public:
    void initialize_new_connect(int client_fd, const sockaddr_in *client_address);
    void close_connet(bool close_flag = true);
    bool read_from_client_socket();
    bool write_respond_to_socket();
    void print_read_buffer();
    void print_write_buffer();
    void process();
};

int Http_Connect::hc_thread_epollfd = -1; // Each thread have a epollfd that belong to itself.
int Http_Connect::hc_client_count = 0;    // Client count.
const char *OK_200_TITLE = "OK";
const char *OK_200_INFORMATION = "<html><body></body></html>";
const char *ERROR_400_TITLE = "Bad request. ";
const char *ERROR_400_INFORMATION = "Your request has a syntax error or is inherently impossible to satisfy.";
const char *ERROR_403_TITLE = "Forbidden.";
const char *Error_403_INFORMATION = "You do not have permission to get file from this server.";
const char *Error_404_TITLE = "Not found.";
const char *Error_404_INFORMATION = "The request file was not found on this server.";
const char *Error_500_TITLE = "Internal error";
const char *Error_500_INFORMATION = "Server has an error when process the request.";
const char *website_root = "/var/tmp/www";

int setnonblocking(int fd)
{
    int old_file_flag;
    int new_file_flag;
    old_file_flag = fcntl(fd, F_GETFL);
    new_file_flag = old_file_flag | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_file_flag);
    return old_file_flag;
}

void add_epoll_fd(int epollfd, int fd, bool one_shot)
{
    struct epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    if (one_shot)
    {
        event.events = event.events | EPOLLONESHOT;
        /* code */
    }
    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonblocking(fd);
}

void remove_epoll_fd(int epollfd, int fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
    close(fd);
}

void modify_epoll_fd(int epollfd, int fd, struct epoll_event *event_pt)
{
    struct epoll_event event;
    event = *event_pt;
    event.data.fd=fd;
    event.events = event.events | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

char *Http_Connect::get_read_buffer_pt()
{
    return hc_read_buffer + hc_current_analysis_line_start_index;
}

Http_Connect::Http_Connect(/* args */)
{
}

void Http_Connect::initialize_new_connect(int client_fd, const sockaddr_in *client_address)
{
    hc_client_socket = client_fd;
    hc_address_client = *client_address;
    int open_flag = 1;
    setsockopt(hc_client_socket, SOL_SOCKET, SO_REUSEADDR, &open_flag, sizeof(open_flag)); // Debug function, remember to comment it.
    add_epoll_fd(hc_thread_epollfd, hc_client_socket, true);
    hc_client_count++;
    initialize();
}

void Http_Connect::initialize()
{
    hc_thread_check_state = THREAD_STATE_ANALYSIS_REQUEST_LINE;
    hc_method = GET;
    memset(hc_read_buffer, '\0', MAX_READ_BUFFER);
    hc_current_read_buffer_next_empty_index = 0;
    hc_current_analysis_char_index = 0;
    hc_current_analysis_line_start_index = 0;
    memset(hc_write_buffer, '\0', MAX_WRITE_BUFFER);
    hc_write_buffer_ready_to_send_count = 0;
    hc_respond_have_send_count = 0;
    hc_respond_ready_to_send_count = 0;
    memset(hc_absolute_file_path, '\0', MAX_FILE_PATH);
    hc_file_pathname = NULL;
    hc_http_version = NULL;
    hc_server_name = NULL;
    hc_request_content_length = 0;
    hc_linger = false;
    test_server = false;
    hc_map_file_address = NULL;
    hc_iovec_memory_number = 0;
}

void Http_Connect::close_connet(bool close_flag)
{
    if (close_flag == true && hc_client_socket != -1)
    {
        remove_epoll_fd(hc_thread_epollfd, hc_client_socket);
        hc_client_socket = -1;
        hc_client_count--;
        /* code */
    }
}

void Http_Connect::print_read_buffer()
{
    std::cout << hc_read_buffer << std::endl;
}

void Http_Connect::print_write_buffer()
{
    std::cout << hc_write_buffer << std::endl;
}

bool Http_Connect::read_from_client_socket()
{
    int current_read_count = 0;
    if (hc_current_read_buffer_next_empty_index >= MAX_READ_BUFFER - 1)
    {
        return false;
        /* code */
    }
    while (hc_current_read_buffer_next_empty_index < MAX_READ_BUFFER - 1)
    {
        current_read_count = recv(hc_client_socket, hc_read_buffer + hc_current_read_buffer_next_empty_index, MAX_READ_BUFFER - hc_current_read_buffer_next_empty_index, 0);
        if (current_read_count == -1)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // std::cout << hc_read_buffer << std::endl;
                break;
                /* code */
            }
            return false;
            /* code */
        }

        if (current_read_count == 0)
        {
            return false;
            /* code */
        }
        hc_current_read_buffer_next_empty_index += current_read_count;
        /* code */
    }
    return true;
}

Http_Connect::READ_LINE_STATE Http_Connect::parse_line()
{
    char temp_char;
    for (; hc_current_analysis_char_index < hc_current_read_buffer_next_empty_index; hc_current_analysis_char_index++)
    {
        temp_char = hc_read_buffer[hc_current_analysis_char_index];
        if (temp_char == '\r') // If the temp_char is '\r' char, analysis next char.
        {
            if ((hc_current_analysis_char_index + 1) == hc_current_read_buffer_next_empty_index)
            {
                // std::cout<<"Positon 290"<<std::endl;
                return LINE_INCOMPLETE;
                /* code */
            }
            else if (hc_read_buffer[hc_current_analysis_char_index + 1] == '\n')
            {
                hc_read_buffer[hc_current_analysis_char_index++] = '\0';
                hc_read_buffer[hc_current_analysis_char_index++] = '\0';
                // std::cout<<"Positon 297"<<std::endl;
                return LINE_INTEGRITY;
            }

            // std::cout<<"Position 299."<<std::endl;
            return LINE_BAD;
        }
        else if (temp_char == '\n')
        {
            if (hc_read_buffer[hc_current_analysis_char_index - 1] == '\r')
            {
                hc_read_buffer[hc_current_analysis_char_index - 1] = '\0';
                hc_read_buffer[hc_current_analysis_char_index++] = '\0';
                return LINE_INTEGRITY;
                /* code */
            }
            std::cout << "Position 311" << std::endl;
            return LINE_BAD;
        }
        else
        {
        }
    }
    std::cout << "Positon 319" << std::endl;
    return LINE_INCOMPLETE;
}

Http_Connect::SERVER_ANALYSIS_RESULT Http_Connect::parse_request_line(char *http_request_text)
{
    hc_file_pathname = strpbrk(http_request_text, "\t"); // Find the first blank of client request line.
    if (hc_file_pathname == NULL)
    {
        return REQUEST_ERROR;
        /* code */
    }
    *hc_file_pathname = '\0';
    hc_file_pathname++;
    hc_file_pathname += strspn(hc_file_pathname, "\t"); // Clear extra spaces;
    char *method = http_request_text;
    if (strncasecmp(method, "GET", 3) == 0)
    {
        hc_method = GET;
    }
    else
    {
        return REQUEST_ERROR;
    }
    hc_http_version = strpbrk(hc_file_pathname, "\t"); // Find the second blank of client request line.
    if (hc_http_version == NULL)
    {
        return REQUEST_ERROR;
        /* code */
    }
    *hc_http_version = '\0';
    hc_http_version++;
    hc_http_version += strspn(hc_http_version, "\t"); // Clear extra spaces.
    if (strncasecmp(hc_http_version, "HTTP/1.1", 8) != 0)
    {
        return REQUEST_ERROR;
        /* code */
    }
    if (strncasecmp(hc_file_pathname, "http://", 7) != 0)
    {
        return REQUEST_ERROR;
    }

    hc_file_pathname += 7;
    hc_file_pathname = strpbrk(hc_file_pathname, "/");

    if (!hc_file_pathname)
    {
        test_server = true;
        hc_thread_check_state = THREAD_STATE_ANALYSIS_HEADER;

        return REQUSET_TEST;
    }

    hc_thread_check_state = THREAD_STATE_ANALYSIS_HEADER;
    return REQUEST_INCOMPLETE;
}

Http_Connect::SERVER_ANALYSIS_RESULT Http_Connect::parse_headers(char *http_request_text) // Analysis http header segment.
{
    if (http_request_text[0] == '\0') // If the first char of http_request_text buffer is '\0' char that indicate the header segment has processed compelete.
    {
        if (hc_request_content_length != 0) // If the hc_request_content_length not equal to 0, we need to process request content.
        {
            hc_thread_check_state = THREAD_STATE_ANALYSIS_CONTENT; // The minor finite state machine change to THREAD_STATE_ANALYSIS_CONTENT state.
            return REQUEST_INCOMPLETE;
        }
        return REQUEST_INTEGRITY; // We get a integrity request.
        /* code */
    }
    else if (strncasecmp(http_request_text, "Connection:", 11) == 0) // Find Connection option.
    {
        http_request_text += 11;
        http_request_text += strspn(http_request_text, "\t"); // Remove extra spaces.
        if (strncasecmp(http_request_text, "keep-Live", 9) == 0)
        {
            hc_linger = true;
            /* code */
        }
        return REQUEST_INCOMPLETE;
        /* code */
    }
    else if (strncasecmp(http_request_text, "Content-Length:", 15) == 0) // Find Content-Length option.
    {
        http_request_text += 15;
        http_request_text += strspn(http_request_text, "\t");
        hc_request_content_length = atoi(http_request_text);
        /* code */
        return REQUEST_INCOMPLETE;
    }
    else if (strncasecmp(http_request_text, "Host:", 5) == 0) // Find Host option.
    {
        http_request_text += 5;
        http_request_text += strspn(http_request_text, "\t");
        hc_server_name = http_request_text;
        /* code */
        return REQUEST_INCOMPLETE;
    }
    else
    {
        std::cout << "Unknow header " << http_request_text << std::endl;
        return REQUEST_ERROR;
    }
}

// The function just to check content that whether is complete.
Http_Connect::SERVER_ANALYSIS_RESULT Http_Connect::parse_content(char *http_request_text)
{
    if (test_server)
    {
        return REQUEST_INTEGRITY;
    }

    if (hc_current_read_buffer_next_empty_index >= hc_current_analysis_char_index + hc_request_content_length)
    {
        http_request_text[hc_request_content_length] = '\0';
        return REQUEST_INTEGRITY;
    }
    return REQUEST_INCOMPLETE;
}

Http_Connect::SERVER_ANALYSIS_RESULT Http_Connect::execute_request()
{
    if (test_server)
    {
        hc_map_file_address = NULL;
        hc_file_state.st_size = 0;
        return REQUSET_TEST;
        /* code */
    }

    strcpy(hc_absolute_file_path, website_root); // Copy website root directory to hc_absolute_file_path buffer that aim to get the file full path.
    int website_root_length = strlen(hc_absolute_file_path);
    strncpy(hc_absolute_file_path + website_root_length, hc_file_pathname, MAX_FILE_PATH - website_root_length - 1); // Copy file relative path to hc_absolute_file_path buffer.
    if (stat(hc_absolute_file_path, &hc_file_state) < 0)                                                             // Get client request file state.
    {
        return SOURCE_NOT_FIND;
        /* code */
    }
    else if (!(hc_file_state.st_mode & S_IROTH))
    {
        return REQUEST_NO_PERMISSION;
        /* code */
    }
    else if (S_ISDIR(hc_file_state.st_mode))
    {
        return REQUEST_ERROR;
        /* code */
    }
    else
    {
        ;
    }
    int request_fd;
    request_fd = open(hc_absolute_file_path, O_RDONLY);
    hc_map_file_address = (char *)mmap(NULL, hc_file_state.st_size, PROT_READ, MAP_PRIVATE, request_fd, 0); // Map file.
    close(request_fd);
    return REQUEST_GET_FILE;
}

void Http_Connect::unmap()
{
    if (hc_map_file_address)
    {
        munmap(hc_map_file_address, hc_file_state.st_size);
        hc_map_file_address = NULL;
        /* code */
    }
}

Http_Connect::SERVER_ANALYSIS_RESULT Http_Connect::analysis_http_request() // Major finite state machine
{
    READ_LINE_STATE line_state = LINE_INTEGRITY;
    SERVER_ANALYSIS_RESULT result = REQUEST_INCOMPLETE;
    char *http_request_text = NULL;
    //
    while (((hc_thread_check_state == THREAD_STATE_ANALYSIS_CONTENT) && (line_state == LINE_INTEGRITY)) || ((line_state = parse_line()) == LINE_INTEGRITY))
    {
        http_request_text = get_read_buffer_pt();
        hc_current_analysis_line_start_index = hc_current_analysis_char_index;
        switch (hc_thread_check_state)
        {
        case THREAD_STATE_ANALYSIS_REQUEST_LINE: // Process request line.
        {
            result = parse_request_line(http_request_text);
            if (result == REQUEST_ERROR)
            {
                return REQUEST_ERROR;
                /* code */
            }

            break;
        }
        case THREAD_STATE_ANALYSIS_HEADER: // Process header.
        {
            result = parse_headers(http_request_text);
            if (result == REQUEST_ERROR)
            {
                return REQUEST_ERROR;
                /* code */
            }
            else if (result == REQUEST_INTEGRITY)
            {
                return execute_request();
                /* code */
            }

            break;
        }
        case THREAD_STATE_ANALYSIS_CONTENT: // Process centent.
        {
            result = parse_content(http_request_text);
            if (result == REQUEST_INTEGRITY)
            {
                return execute_request();
                /* code */
            }
            // line_state = LINE_INCOMPLETE;
            return REQUEST_INCOMPLETE;
        }
        default:
            return ERROR_INTERNAL;
        }
        /* code */
    }
    if (line_state == LINE_BAD)
    {
        return REQUEST_ERROR;
    }

    return REQUEST_INCOMPLETE;
}

bool Http_Connect::add_server_respond(int number, ...)
{
    if (hc_write_buffer_ready_to_send_count >= MAX_WRITE_BUFFER - 1)
    {
        return false;
    }
    va_list argument_list;
    const char *format;
    va_start(argument_list, number);
    format = va_arg(argument_list, const char *);
    int function_flag = vsnprintf(hc_write_buffer + hc_write_buffer_ready_to_send_count, MAX_WRITE_BUFFER - hc_write_buffer_ready_to_send_count, format, argument_list);
    if (function_flag >= MAX_WRITE_BUFFER - hc_write_buffer_ready_to_send_count)
    {
        return false;
        /* code */
    }
    hc_write_buffer_ready_to_send_count += function_flag;
   // std::cout<<"hc_write_buffer_ready_to_send_count"<<hc_write_buffer_ready_to_send_count<<std::endl;
    va_end(argument_list);
    return true;
}

bool Http_Connect::add_content(const char *content)
{
    if (add_server_respond(2, "%s", content))
    {
        return true;
    }
    std::string error_string = "Http_Connect::add_content function error.\n";
    throw error_information(error_string);
}

bool Http_Connect::add_respond_line(int state_code, const char *state_title)
{
    if (add_server_respond(4, "%s\t%d\t%s\r\n", "Http/1.1", state_code, state_title))
    {
        return true;
    }
    std::string error_string = "Http_Connect::add_respond_line function error.\n";
    throw error_information(error_string);
}

bool Http_Connect::add_content_length(int content_length)
{
    if (add_server_respond(2, "Content-Length: %d\r\n", content_length))
    {
        return true;
    }
    std::string error_string = "Http_Connect::add_content_length function error.\n";
    throw error_information(error_string);
}

bool Http_Connect::add_linger()
{
    if (add_server_respond(2, "Connection: %s\r\n", (hc_linger == true ? "keep-live" : "close")))
    {
        return true;
    }
    std::string error_string = "Http_Connect::add_linger function error.\n";
    throw error_information(error_string);
}

bool Http_Connect::add_blank_line()
{
    if (add_server_respond(2, "%s", "\r\n"))
    {
        return true;
    }
    std::string error_string = "Http_Connect::add_blank_line function error.\n";
    throw error_information(error_string);
}

bool Http_Connect::add_headers(int content_length)
{
    add_content_length(content_length);
    add_linger();
    add_blank_line();
    return true;
}

bool Http_Connect::write_http_respond(SERVER_ANALYSIS_RESULT result)
{
    switch (result)
    {
    case ERROR_INTERNAL:
    {
        add_respond_line(500, Error_500_TITLE);
        add_headers(strlen(Error_500_INFORMATION));
        if (!add_content(Error_500_INFORMATION))
        {
            return false;
        }
        break;
    }
    case REQUEST_ERROR:
    {
        add_respond_line(400, ERROR_400_TITLE);
        add_headers(strlen(ERROR_400_INFORMATION));
        if (!add_content(ERROR_400_INFORMATION))
        {
            return false;
        }
        break;
    }
    case SOURCE_NOT_FIND:
    {
        add_respond_line(404, Error_404_TITLE);
        add_headers(strlen(Error_404_INFORMATION));
        if (!add_content(Error_404_INFORMATION))
        {
            return false;
        }
        break;
    }
    case REQUEST_NO_PERMISSION:
    {
        add_respond_line(403, ERROR_403_TITLE);
        add_headers(strlen(Error_403_INFORMATION));
        if (!add_content(Error_403_INFORMATION))
        {
            return false;
        }
        break;
    }
    case REQUEST_GET_FILE:
    {
        add_respond_line(200, OK_200_TITLE);
        if (hc_file_state.st_size != 0)
        {
            add_headers(hc_file_state.st_size);
            hc_iovector[0].iov_base = hc_write_buffer;
            hc_iovector[0].iov_len = hc_write_buffer_ready_to_send_count ;  
            hc_iovector[1].iov_base = hc_map_file_address;
            hc_iovector[1].iov_len = hc_file_state.st_size;
            hc_iovec_memory_number = 2;
            hc_respond_ready_to_send_count = hc_write_buffer_ready_to_send_count + hc_file_state.st_size;
            return true;
            /* code */
        }
        else
        {
            add_headers(strlen(OK_200_INFORMATION));
            if (!add_content(OK_200_INFORMATION))
            {
                return false;
                /* code */
            }
        }
        break;
    }
    case REQUSET_TEST:
    {
        add_respond_line(200, OK_200_TITLE);
        add_headers(strlen(OK_200_INFORMATION));
        if (!add_content(OK_200_INFORMATION))
        {
            return false;
        }
        break;
    }
    default:
    {
        return false;
    }
    }

    hc_iovector[0].iov_base = hc_write_buffer;
    hc_iovector[0].iov_len = hc_write_buffer_ready_to_send_count ;
    hc_iovec_memory_number = 1;
    hc_respond_ready_to_send_count = hc_write_buffer_ready_to_send_count ;
    //std::cout<<"hc_respond_ready_to_send_count "<<hc_respond_ready_to_send_count<<std::endl;
    return true;
}

bool Http_Connect::write_respond_to_socket()
{
    int function_flag = 0;
    //std::cout<<"hc_respond_ready_to_send_cout"<<hc_respond_ready_to_send_count<<std::endl;
    if (hc_respond_ready_to_send_count == 0) // There is no data in the respond buffer(hc_respond_ready_to_send_count), we change epoll event to listen EPOLLIN event.
    {
        struct epoll_event event;
        event.events = EPOLLIN;
        modify_epoll_fd(hc_thread_epollfd, hc_client_socket, &event);
        initialize();
        return true;
        
    }
    print_write_buffer();

    while (true)
    {
        function_flag = writev(hc_client_socket, hc_iovector, hc_iovec_memory_number);
        std::cout<<"Write byte count: "<<function_flag <<std::endl;
        if (function_flag < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) // If there is no additional space in tcp buffer, we wait next epoll event(EPOLLOUT), during this time, we can't accept next client request.
            {
                struct epoll_event event;
                event.events = EPOLLOUT;
                modify_epoll_fd(hc_thread_epollfd, hc_client_socket, &event);
                return true;
                /* code */
            }

            unmap();
            return false;
        }

        hc_respond_have_send_count += function_flag;
        hc_respond_ready_to_send_count -= function_flag;
        if (hc_respond_ready_to_send_count <= 0) // When server respond send successfully, we according the Connection segment to decide whether close connection.
        {
            struct epoll_event event;
            event.events = EPOLLIN;
            unmap();
            if (hc_linger)
            {
                initialize();
                modify_epoll_fd(hc_thread_epollfd, hc_client_socket, &event);
                return true;
                /* code */
            }
            else
            {
                return false;
            }
        }
    }
}

void Http_Connect::process()
{
    SERVER_ANALYSIS_RESULT result = analysis_http_request();
    struct epoll_event event;

    if (result == REQUEST_INCOMPLETE)
    {
        event.events = EPOLLIN;

        modify_epoll_fd(hc_thread_epollfd, hc_client_socket, &event);
        return;
        /* code */
    }
    if (result == REQUEST_ERROR)
    {
        close_connet(true);
        return;
    }

    /*if (result==REQUSET_TEST)
    {

    }*/

    bool function_flag = write_http_respond(result);

    if (!function_flag)
    {
        close_connet(true);
        
        return;
        /* code */
    }
    event.events = EPOLLOUT;
    modify_epoll_fd(hc_thread_epollfd, hc_client_socket, &event);
    return;
}
Http_Connect::~Http_Connect()
{
}
#endif
