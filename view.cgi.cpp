#include <iostream>
#include <fstream>
#include <string>
#include <experimental/filesystem>

#include <boost/format.hpp>
#include <fmt/ostream.h>
#include <fmt/format.h>

using namespace std;
namespace fs = experimental::filesystem;

int main(int argc, char *argv[])
{
    cout << "Content-Type: text/html" << "\r\n\r\n";
    cout << fstream{"./static/part1"}.rdbuf();

    const string path = "./upload";
    int i = 1;
    for (const auto& entry : fs::directory_iterator(path)) {
        const auto filename = entry.path().filename().string();
        
        auto ftime = fs::last_write_time(entry.path());
        std::time_t cftime = decltype(ftime)::clock::to_time_t(ftime);
        std::tm *gmt = std::localtime(&cftime);
        char buf[80]{};
	    strftime(buf, 80, "%F %H:%M:%S", gmt);
        const string updatedTime {buf};

        cout << fmt::format(
            "<tr>\r\n \
                <th scope=\"row\">{}</th>\r\n \
                <td><a href=\"view.cgi/?file={}\">{}</a></td>\r\n \
                <td>{}</td>\r\n \
            </tr>\r\n", i++, filename, filename, updatedTime);   
    }
    cout << fstream{"./static/part2"}.rdbuf();
}

