#pragma once

#include <stdio.h>
#include <iostream>
#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <fstream>

// Custom comparator
// Sorts file names in alpha-numeric order:
// [9, 33, 222, 1111] instead of [1111, 222, 33, 9]
struct myCmp
{
    bool operator() (const std::string &a, const std::string &b) const
    {
        auto isDigit = [](char ch) -> bool
        {
            return (ch > 47 && ch < 58);
        };

        auto getNum = [&isDigit](const char *ch) -> int
        {
            char buf[64];
            int i = 0;

            while (isDigit(*ch))
            {
                buf[i++] = *ch;
                ch++;
            }

            buf[i] = '\0';

            return atoi(buf);
        };

        const char* ch1 = a.c_str();
        const char* ch2 = b.c_str();

        while (1)
        {
            char c1 = *ch1;
            char c2 = *ch2;

            if (isDigit(c1) && isDigit(c2))
            {
                // Both are digits. Get numeric values and compare:
                int n1 = getNum(ch1);
                int n2 = getNum(ch2);

                if (n1 != n2)
                    return n1 < n2;
            }
            else
            {
                if (c1 != c2)
                {
                    return c1 < c2;
                }
            }

            // Skip both.
            // This is safe, as we compare file names that come from the same directory => are unique
            ch1++;
            ch2++;
        }

        return a < b;
    }
};



class myApp
{
	public:

		struct fileInfo
		{
			std::string fullName;
			size_t		size;
            bool        canRename;
            int         width;
            int         height;
		};

        struct dirInfo
        {
            std::string                                     fullName;
            std::map<std::string, myApp::fileInfo, myCmp>   files;
        };

	
		enum DIRS { RESIZE, QUALITY };

	public:

		myApp(const char* dir) : _dir(dir)
		{
			if (_dir.back() != '\\')
				_dir += "\\";

			createAuxDirs();
		}

        void    readFiles();
        void    renameFiles();
        void    sortDirs();

static  bool pathValid(std::string dir)
        {
            DWORD dwAttrib = GetFileAttributesA(dir.c_str());
            return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
        };

	private:

		void    createAuxDirs   ();
        void	findDirs        ();
        void	findFiles       (std::string, std::map<std::string, myApp::fileInfo, myCmp> &);
        bool	GetImageSize    (const char*, int&, int&);
        bool	dirExists       (std::string);
        void	mkDir           (std::string);
        void	mvDir           (std::string, std::string, enum DIRS);

	private:

		std::string _dir;
		std::string _dirResize;
		std::string _dirQuality;

        std::map<std::string, dirInfo> _mapDirs;
};
