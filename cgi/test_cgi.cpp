#include <iostream>
#include <string>
#include <cstdlib>

int main()
{
    std::string method = getenv("METHOD");
    std::cerr << "for test: method = " << method << std::endl;
    if (method == "GET")
    {
        std::cerr << "for test: query string= " << getenv("QUERY_STRING") << std::endl;
    }
    else if (method == "POST")
    {

    }
    else
    {

    }
    return 0;
}