#include "myApp.h"

#define MY_DEBUG
#undef MY_DEBUG

// -----------------------------------------------------------------------------------------------------------

int main(int argc, char** argv)
{
    std::string path;

    // Get path from the argument, if present
    for (int i = 0; i < argc; i++)
    {
        std::string param(argv[i]);

        if (param.substr(0, 6) == "/path=")
            path = param.substr(6, param.length());
    }

#if defined MY_DEBUG
    path = "d:\\test";
    path = "i:\\bbb";
#endif

    if (!path.empty() && myApp::pathValid(path))
    {
        myApp app(path.c_str());

        app.readFiles();

        // Rename files (optional, will be asked about it)
        app.renameFiles();

        // Move directories
        app.sortDirs();

        std::cout << " Done: OK" << std::endl;
    }
    else
    {
        std::cout << " Path is empty or does not exist: '" << path << "'" << std::endl;
        std::cout << " Done: Fail" << std::endl;
    }

#if defined MY_DEBUG
    getchar();
    getchar();
#endif

    return 0;
}

// -----------------------------------------------------------------------------------------------------------
