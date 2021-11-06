#include <iostream>

int main(int argc, char *argv[])
{
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0]
                  << " <listen_adrr> <connect_addr>" << std::endl;
        std::cerr << "Example: " << argv[0] << " 127.0.0.1:1234 1.2.3.4:8080" << std::endl;
        return EXIT_FAILURE;
    }
    std::string listenAddr(argv[1]);
    std::string connectAddr(argv[2]);


    return 0;
}