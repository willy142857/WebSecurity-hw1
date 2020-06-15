Simple Web Server
===

## 建置環境與使用說明

* Ubuntu 18.04
* g++ 7.5
* boost 1.65
* GNU Make 4.1

```bash
$ sudo apt -y update
$ sudo apt install -y g++ libboost-all-dev make
$ make && ./server 8080 # listen on port 8080
```

重要程式碼
---

```cpp
io_service global_io_service; // 處理所有 asynchronous operations
...
int main(int argc, char *argv[])
{    
    // do something

    // block until all asynchronous operations have finished. 
    // While the server is running, there is always at least one
    // asynchronous operation outstanding: the asynchronous accept
    // call waiting for new incoming connections.
    global_io_service.run();
    
    // do something
}
```

```cpp
void readFirstLine() {
    auto self(shared_from_this()); // async function會立刻return，直到完成後才會回來執行，因此需要確保object的lifecycle
    asio::async_read_until(socket_, buf, '\r',
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
```
透過pipe把post data傳給cgi，並將STDOUT_FILENO導向socket
```cpp
void response() {
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
        exit(0);
    }
}
```

設計架構與功能
---

### 架構

![](https://i.imgur.com/JdnKgvS.jpg)

* Client
    * 就是個browser
* Server
    * 採用**asynchronous socket**，提升效能
    * 解析request和setenv
    * 將stdout導向socket，fork並執行對應的cgi
* Cgi
    * 從env拿到資料，把要輸出的資料全部丟到stdout就對了

### 功能

使用者透過<hostname>/view.cgi進入後，可以看到目前上傳的檔案，並可以點入觀看詳細資料，最下方有上傳選項，使用者可以選擇本機的檔案，按下Upload後，會送Post Request到Server，Server會呼叫insert.cgi把檔案寫入upload資料夾中，成功後5秒內會將畫面重新導回/view.cgi。


成果截圖
---

| ![view](https://i.imgur.com/x6c8TN0.png) | 一開始的畫面 |
| -------- | -------- |
| ![preupload](https://i.imgur.com/6GtcCtH.png) | 選擇檔案 |
| ![uploaded](https://i.imgur.com/csoi3hI.png) | 上傳成功 |
| ![view result](https://i.imgur.com/veo3uvn.png) | 清單新增了 |
| ![png](https://i.imgur.com/ACCRKCt.png) | 查看剛剛的企鵝 |
| ![pdf](https://i.imgur.com/gYDd7Wo.jpg) | 或者看個HW1 |
| ![console](https://i.imgur.com/oTlth2F.png) | console和upload資料夾 |

## 困難與心得

這次作業中，最大的困難是太久沒寫C/C++了，以及被windows荼毒久了，fork、pipe、dup2、socket等linux function花了不少時間研究，總之作業系統學的都差不多忘光了。
而另一個困難點是採用asynchronous實作，思考邏輯和平常寫程式不太一樣，不過完成時成就感蠻高的，並更了解web server到底在幹嘛，以及在框架面前，很少接觸的cgi program。
