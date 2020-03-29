#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <cstdlib>
#include "3rd-party/fmt/include/color.h"

#include "./utility.cpp"

using namespace boost;
using boost::asio::io_service;
using boost::asio::ip::tcp;
io_service global_io_service;

/// A request received from a client.
class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(tcp::socket socket) : socket_(std::move(socket)) {}

    void start() { readFirstLine(); }
private:
    tcp::socket socket_;
    asio::streambuf buf;
    char status_str[200] = "HTTP/1.1 200 OK\n";
    std::unordered_map<std::string, std::string> headers;
    std::string payload;
    std::string REQUEST_METHOD;
    std::string REQUEST_URI;
    std::string QUERY_STRING;
    std::string SERVER_PROTOCOL;
    char HTTP_HOST[100];
    char SERVER_ADDR[100];
    char SERVER_PORT[10];
    char REMOTE_ADDR[100];
    char REMOTE_PORT[10];
    std::string SCRIPT_NAME = "./";
    char blackhole[100];

    void readFirstLine()
    {
        auto self(shared_from_this());
        asio::async_read_until(
            socket_, buf, '\r',
            [this, self](const system::error_code &ec, std::size_t s) {
                std::string line, ignore;
                std::istream stream{&buf};
                std::getline(stream, line, '\r');
                std::getline(stream, ignore, '\n');

                if(line.empty())
                    return;

                std::stringstream{line} >> REQUEST_METHOD >> REQUEST_URI >> SERVER_PROTOCOL;
                const auto pos = REQUEST_URI.find('?');
                if (pos != std::string::npos) {
                    QUERY_STRING = REQUEST_URI.substr(pos + 1);
                    REQUEST_URI = REQUEST_URI.substr(0, pos);
                }
                SCRIPT_NAME = "." + REQUEST_URI;
                
                readNextLine();
            });
    }

    void readNextLine()
    {
        auto self(shared_from_this());
        asio::async_read_until(
            socket_, buf, '\r',
            [this, self](const system::error_code &ec, std::size_t s) {
                std::string line, ignore;
                std::istream stream{&buf};
                std::getline(stream, line, '\r');
                std::getline(stream, ignore, '\n');
                
                if (line.empty()) {
                    if (headers.count("Content-Length") == 0 || headers["Content-Length"] == "0")
                        doWrite(55688);
                    else {
                        readBody(std::stoi(headers["Content-Length"]) - buf.size());
                    }
                }
                else {
                    const auto pos = line.find(':');
                    headers.emplace(line.substr(0, pos), line.substr(pos + 2));
                    readNextLine();
                }
            });
    }

    void readBody(int length)
    {
        auto self(shared_from_this());
        asio::async_read(socket_, buf, asio::transfer_exactly(length), 
            [this, self](const system::error_code& ec, std::size_t s) {
                std::istream stream{&buf};
                std::stringstream ss;
                ss << stream.rdbuf();
                payload = ss.str();
                
                if (!ec)
                    doWrite(55688);
            });
    }

    void doWrite(std::size_t length)
    {
        auto self(shared_from_this());
        boost::asio::async_write(
            socket_, boost::asio::buffer(status_str, strlen(status_str)),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    strcpy(SERVER_ADDR,
                           socket_.local_endpoint().address().to_string().c_str());
                    sprintf(SERVER_PORT, "%u", socket_.local_endpoint().port());
                    strcpy(REMOTE_ADDR,
                           socket_.remote_endpoint().address().to_string().c_str());
                    sprintf(REMOTE_PORT, "%u", socket_.remote_endpoint().port());

                    fmt::print(fmt::fg(fmt::color::yellow), "[{}]\n", timestamp());
                    fmt::print(fmt::fg(fmt::color::light_sea_green),"Connection: {}:{}\n", REMOTE_ADDR, REMOTE_PORT);
                    fmt::print(fmt::fg(fmt::color::azure), "Method: {}\n", REQUEST_METHOD);
                    fmt::print(fmt::fg(fmt::color::azure), "Uri: {}\n", REQUEST_URI);

                    setEnv();
                    response();

                    readFirstLine();
                }
            });
    }

    void setEnv()
    {
        setenv("REQUEST_METHOD", REQUEST_METHOD.c_str(), 1);
        setenv("REQUEST_URI", REQUEST_URI.c_str(), 1);
        setenv("QUERY_STRING", QUERY_STRING.c_str(), 1);
        setenv("SERVER_PROTOCOL", SERVER_PROTOCOL.c_str(), 1);
        setenv("HTTP_HOST", HTTP_HOST, 1);
        setenv("SERVER_ADDR", SERVER_ADDR, 1);
        setenv("SERVER_PORT", SERVER_PORT, 1);
        setenv("REMOTE_ADDR", REMOTE_ADDR, 1);
        setenv("REMOTE_PORT", REMOTE_PORT, 1);
        setenv("SCRIPT_NAME", SCRIPT_NAME.c_str(), 1);
        if (REQUEST_METHOD == "POST") {
            setenv("CONTENT_LENGTH", headers["Content-Length"].c_str(), 1);
        }
    }

    void response()
    {
        int pfd[2];
        if (pipe(pfd) < 0)
            fmt::print(stderr, fmt::fg(fmt::color::red), "pipe error\n");
        
        global_io_service.notify_fork(io_service::fork_prepare);
        if (fork() != 0) {
            global_io_service.notify_fork(io_service::fork_parent);
            close(pfd[0]);
            if (headers.count("Content-Length"))
                write(pfd[1], payload.data(), payload.length());
            close(pfd[1]);
            socket_.close();
        }
        else {
            global_io_service.notify_fork(io_service::fork_child);
            int sock = socket_.native_handle();
            dup2(sock, STDOUT_FILENO);
            socket_.close();
            
            close(pfd[1]);
            dup2(pfd[0], STDIN_FILENO);
            close(pfd[0]);

            if (REQUEST_METHOD == "GET" && REQUEST_URI == "/view.cgi/" &&
                QUERY_STRING.find("file=") != std::string::npos)
                execlp("./viewstatic.cgi", "./viewstatic.cgi", NULL);
            else if (SCRIPT_NAME == "./view.cgi" || SCRIPT_NAME == "./insert.cgi")
                if (execlp(SCRIPT_NAME.c_str(), SCRIPT_NAME.c_str(), NULL) < 0)
                    std::cout << "Content-type:text/html\r\n\r\n<h1>Not Found</h1>";
        }
    }
};

class WebServer
{
public:
    WebServer(short port)
        : acceptor_(global_io_service, tcp::endpoint(tcp::v4(), port)),
          socket_(global_io_service)
    {
        accpet();
    }

private:
    tcp::acceptor acceptor_;
    tcp::socket socket_;

    void accpet()
    {
        acceptor_.async_accept(socket_, [this](system::error_code ec) {
            if (!ec) {
                std::make_shared<Session>(std::move(socket_))->start();
            }

            accpet();
        });
    }
};

int main(int argc, char *argv[])
{
    try {
        if (argc != 2) {
            fmt::print(fmt::fg(fmt::color::antique_white), "Usage: server <port>\n");
            return 1;
        }

        fmt::print(fmt::fg(fmt::color::light_cyan), "Server is listening on port:{}\n", argv[1]);
        WebServer server(std::atoi(argv[1]));

        global_io_service.run();
    }
    catch (const std::exception& e) {
        fmt::print(fmt::fg(fmt::color::dark_red), "Exception: {}", e.what());
    }

    return 0;
}