/* By Abdullah As-Sadeed */

/*
g++ ./Sadeed_Port_Scanner.cpp -o ./Sadeed_Port_Scanner
*/

#include "csignal"
#include "cstring"
#include "fcntl.h"
#include "iostream"
#include "mutex"
#include "netdb.h"
#include "poll.h"
#include "thread"
#include "vector"

#define TERMINAL_TITLE_START "\033]0;"
#define TERMINAL_TITLE_END "\007"

#define TERMINAL_ANSI_COLOR_RED "\x1b[31m"
#define TERMINAL_ANSI_COLOR_GREEN "\x1b[32m"
#define TERMINAL_ANSI_COLOR_YELLOW "\x1b[33m"
#define TERMINAL_ANSI_COLOR_RESET "\x1b[0m"

#define MAXIMUM_PORTS 65536

std::mutex open_ports_mutex;
int open_ports[MAXIMUM_PORTS], open_ports_count = 0;

void Print_Open_Ports()
{
    if (open_ports_count > 0)
    {
        printf(TERMINAL_ANSI_COLOR_GREEN "\nFound open ports:\n");

        for (int i = 0; i < open_ports_count; i++)
        {
            for (int j = i + 1; j < open_ports_count; j++)
            {
                if (open_ports[i] > open_ports[j])
                {
                    int temporary = open_ports[i];
                    open_ports[i] = open_ports[j];
                    open_ports[j] = temporary;
                }
            }
        }

        for (int i = 0; i < open_ports_count; ++i)
        {
            printf("%d\n", open_ports[i]);
        }
        printf(TERMINAL_ANSI_COLOR_RESET);
    }
    else
    {
        printf(TERMINAL_ANSI_COLOR_GREEN "\nNo open ports found.\n" TERMINAL_ANSI_COLOR_RESET);
    }
}

void Handle_Signal(int signal)
{
    if (signal == SIGINT)
    {
        printf(TERMINAL_ANSI_COLOR_RED "\n\nYou interrupted me by SIGINT signal.\n" TERMINAL_ANSI_COLOR_RESET);

        Print_Open_Ports();

        exit(signal);
    }
}

void Set_NonBlocking(int socket_descriptor)
{
    fcntl(socket_descriptor, F_SETFL, fcntl(socket_descriptor, F_GETFL) | O_NONBLOCK);
}

void Scan_Port_Range(const char *target, int start_port, int end_port)
{
    struct sockaddr_in server;
    int socket_descriptor;

    socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_descriptor == -1)
    {
        perror(TERMINAL_ANSI_COLOR_RED "Could not create socket" TERMINAL_ANSI_COLOR_RESET);
        return;
    }

    Set_NonBlocking(socket_descriptor);

    struct hostent *target_host = gethostbyname(target);
    if (target_host == NULL)
    {
        perror(TERMINAL_ANSI_COLOR_RED "Failed to resolve hostname" TERMINAL_ANSI_COLOR_RESET);
        return;
    }

    server.sin_family = AF_INET;
    server.sin_addr = *((struct in_addr *)target_host->h_addr);

    for (int port = start_port; port <= end_port; ++port)
    {
        server.sin_port = htons(port);

        if (connect(socket_descriptor, (struct sockaddr *)&server, sizeof(server)) < 0)
        {
            if (errno == EINPROGRESS || errno == EWOULDBLOCK)
            {
                struct pollfd fd;
                fd.fd = socket_descriptor;
                fd.events = POLLOUT;

                if (poll(&fd, 1, 1000) > 0 && (fd.revents & POLLOUT))
                {
                    int error = 0;
                    socklen_t error_len = sizeof(error);
                    getsockopt(socket_descriptor, SOL_SOCKET, SO_ERROR, &error, &error_len);

                    if (error == 0)
                    {
                        std::lock_guard<std::mutex> lock(open_ports_mutex);
                        open_ports[open_ports_count++] = port;

                        printf("Thread %lu found open port: " TERMINAL_ANSI_COLOR_GREEN "%d\n" TERMINAL_ANSI_COLOR_RESET, std::this_thread::get_id(), port);
                    }
                    else
                    {
                        // fprintf(stderr, "Thread %lu encountered an error on port %d: %s\n", std::this_thread::get_id(), port, strerror(error));
                    }
                }
                else
                {
                    perror(TERMINAL_ANSI_COLOR_RED "Polling error" TERMINAL_ANSI_COLOR_RESET);
                }
            }
            else
            {
                perror(TERMINAL_ANSI_COLOR_RED "Connection error" TERMINAL_ANSI_COLOR_RESET);
            }
        }

        close(socket_descriptor);
        socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);
        Set_NonBlocking(socket_descriptor);
    }
}

int main(int argument_count, char *argument_values[])
{
    signal(SIGINT, Handle_Signal);

    printf(TERMINAL_TITLE_START "Sadeed Port Scanner" TERMINAL_TITLE_END);

    if (argument_count != 2)
    {
        printf(TERMINAL_ANSI_COLOR_YELLOW "Usage: %s <IP or domain>\n" TERMINAL_ANSI_COLOR_RESET, argument_values[0]);
        return 1;
    }

    const char *target = argument_values[1];
    int thread_count = std::thread::hardware_concurrency();
    std::vector<std::thread> threads;

    printf(TERMINAL_TITLE_START "Sadeed Port Scanner: scanning %s" TERMINAL_TITLE_END, target);
    printf("Using %d threads to scan %s\n\n", thread_count, target);

    for (int i = 0; i < thread_count; ++i)
    {
        int start_port = i * (MAXIMUM_PORTS / thread_count);
        int end_port = (i + 1) * (MAXIMUM_PORTS / thread_count) - 1;

        threads.emplace_back(Scan_Port_Range, target, start_port, end_port);
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    Print_Open_Ports();

    printf(TERMINAL_TITLE_START "Sadeed Port Scanner: scanned %s" TERMINAL_TITLE_END, target);

    return 0;
}
