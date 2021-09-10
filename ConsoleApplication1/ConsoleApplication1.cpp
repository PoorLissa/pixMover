#include "myApp.h"

// -----------------------------------------------------------------------------------------------------------

int main(int argc, char** argv)
{
    std::string path;

    myApp::extractPath(argc, argv, path);

#if defined _DEBUG
    path = "D:\\test\\[__do_resize]\\ViksiQ - Release";
    path = "d:\\test";
    path = "C:\\_maxx\\test\\001";
    path = "C:\\_maxx\\test\\[__do_resize]\\001";
    path = "C:\\_maxx\\test";
#endif

    if (!path.empty() && myApp::pathValid(path))
    {
        float   threshold_dimension = 1.4f;
        float   threshold_size      = 8.0f;
        float   threshold_quality   = 1.5f;
        size_t  threshold_resize    = 3000u;
        size_t  threshold_cover     = 1280u;

        myApp app(path.c_str(), threshold_dimension, threshold_size, threshold_quality, threshold_resize, threshold_cover);

        app.process();

        std::cout << " Press any key to exit : " << std::endl;
        std::ignore = getchar();
    }
    else
    {
        std::cout << " Path is empty or does not exist: '" << path << "'" << std::endl;
        std::cout << " Done: Fail" << std::endl;
    }

#if defined _DEBUG
    std::ignore = getchar();
#endif

    return 0;
}

// -----------------------------------------------------------------------------------------------------------
