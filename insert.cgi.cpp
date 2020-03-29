#include <iostream>
#include <fstream>
#include <experimental/filesystem>

using namespace std;
namespace fs = experimental::filesystem;

int main(int argc, char *argv[])
{
    string rawData;
    char buf[4096];
    while (cin) {
        cin.read(buf, sizeof(buf));
        rawData.append(buf, cin.gcount());
    }

    const string delimiter = "filename=\"";
    auto start = rawData.find(delimiter) + delimiter.length();
    auto end = rawData.rfind("\"\r\n");
    auto filename = rawData.substr(start, end - start);
  
    start = rawData.find("\r\n\r\n") + 4;
    end = rawData.rfind("------");
    auto fileContent = rawData.substr(start, end - start);

    ofstream fout("./upload/" + filename, ios::binary);
    fout << fileContent;
    fout.close();

    cout << "Content-Type: text/html" << "\r\n";
    cout << "Content-Length: " << fs::file_size("./static/done.html") << "\r\n\r\n";
    cout << fstream{"./static/done.html"}.rdbuf();
}