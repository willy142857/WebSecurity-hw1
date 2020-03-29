#include <iostream>
#include <fstream>
#include <experimental/filesystem>

#include "./utility.cpp"

using namespace std;
namespace fs = experimental::filesystem;

int main(int argc, char *argv[])
{
    const auto filename = string{getenv("QUERY_STRING")}.substr(5);
    cout << "Content-Type: " << mineType(filename) << "\r\n";
    cout << "Content-Length: " << fs::file_size("./upload/" + filename) << "\r\n\r\n";
    
    fstream fin("./upload/" + filename);
    if(!fin)
        cout <<"Not Found";
    else
        cout << fin.rdbuf();
}