#include "myApp.h"

// -----------------------------------------------------------------------------------------------------------

int main(int argc, char** argv)
{
    std::string path;

    myApp::extractPath(argc, argv, path);

#if defined _DEBUG
    path = "C:\\_maxx\\test\\001";
    path = "d:\\test";
    path = "D:\\test\\[__do_resize]\\ViksiQ - Release";
#endif

    if (!path.empty() && myApp::pathValid(path))
    {
        myApp app(path.c_str());
        app.process();
    }
    else
    {
        std::cout << " Path is empty or does not exist: '" << path << "'" << std::endl;
        std::cout << " Done: Fail" << std::endl;
    }

#if defined _DEBUG
    getchar();
#endif

    return 0;
}

// -----------------------------------------------------------------------------------------------------------
