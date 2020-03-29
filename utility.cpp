#include <ctime>
#include <string>

std::string timestamp()
{
    std::time_t t = std::time(0);
	std::tm* now = std::localtime(&t);
	char buf[80]{};
	strftime(buf, 80, "%F %H:%M:%S", now);
	return std::string(buf);
}

std::string mineType(const std::string filename) 
{
	const auto pos = filename.rfind(".");
	const auto ext = filename.substr(pos + 1);
	if (ext == "html") return "text/html";
    else if (ext == "htm") return "text/html";
    else if (ext == "txt") return "text/plain";
    else if (ext == "css") return "text/css";
    else if (ext == "gif") return "image/gif";
    else if (ext == "jpg") return "image/jpeg";
    else if (ext == "png") return "image/png";
    else if (ext == "bmp") return "image/x-ms-bmp";
    else if (ext == "doc") return "application/msword";
    else if (ext == "pdf") return "application/pdf";
    else if (ext == "mp4") return "video/mp4";
    else if (ext == "swf") return "application/x-shockwave-flash";
    else if (ext == "swfl") return "video/mp4";
    else if (ext == "ogg") return "audio/ogg";
    else if (ext == "bz2") return "application/x-bzip2";
    else if (ext == "gz") return "application/x-gzip";
    else if (ext == "tar.gz") return "application/x-gzip";
    else return "text/plain";
}