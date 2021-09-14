#pragma once

#include <stdio.h>
#include <iostream>
#include <windows.h>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <fstream>

// Custom comparator for the map container
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
            fileInfo() : fullName(), size(0u), canRename(false), isResized(false), width(0), height(0)
            {
            }

			std::string fullName;
			size_t		size;
            bool        canRename;
            bool        isResized;
            int         width;
            int         height;
		};

        struct dirInfo
        {
            dirInfo() : mode(0), fullName(), avg_size(0.0f), avg_width(0.0f), avg_height(0.0f), median_size(0u), median_width(0), median_height(0)
            {
            }

            std::string                                     fullName;
            std::map<std::string, myApp::fileInfo, myCmp>   files;
            char                                            mode;
            float                                           avg_size;
            float                                           avg_width;
            float                                           avg_height;
            size_t                                          median_size;
            int                                             median_width;
            int                                             median_height;
        };

	
		enum class DIRS { RESIZE, QUALITY, RESIZED, SKIPPED };
        enum       MODE { EXIT, REARRANGE, ANALYZE, RESTORE };

	public:

		myApp(const char* dir, float th_dim, float th_size, float th_qlty, size_t th_res, size_t th_cvr)
            : _dir(dir),
              _mode(MODE::EXIT),
              _threshold_dimension(th_dim),
              _threshold_size(th_size),
              _threshold_quality(th_qlty),
              _threshold_resize(th_res),
              _threshold_cover(th_cvr)
		{
			if (_dir.back() != '\\')
				_dir += "\\";

            std::cout << " myApp::myApp has started" << std::endl;
            std::cout << " Current path is: " << _dir << "\n" << std::endl;
		}

        void process();

static  bool pathValid(std::string dir)
        {
            DWORD dwAttrib = GetFileAttributesA(dir.c_str());
            return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
        };

static  void extractPath(int argc, char** argv, std::string& path)
        {
            // Get path from the argument, if present
            for (int i = 0; i < argc; i++)
            {
                std::string param(argv[i]);

                if (param.substr(0, 6) == "/path=")
                    path = param.substr(6, param.length());
            }
        }

	private:

        void    selectUserOption();
		void    createAuxDirs   (bool);
        void	findDirs        (std::string);
        void	findFiles       (std::string, dirInfo &);
        bool	GetImageSize    (const char*, int&, int&);
        bool	dirExists       (std::string);
        bool	fileExists      (std::string);
        void	mkDir           (std::string);
        void	mvDir           (std::string, std::string, enum class DIRS);
        void    readFiles       (std::string);
        void    renameFiles     ();
        void    renFiles        (dirInfo &, bool, bool, std::string &);
        void    sortDirs        ();
        void    checkOnRenamed  ();
        void    restoreFileNames();
        void    logError        (const std::string &);

        int     rstr_CheckFileStructure     (std::vector<std::string> &);
        bool    rstr_GetHistory             (std::vector<std::string> &, std::string &);
        bool    rstr_ParseInfoVec           (std::vector<std::string> &, std::map<std::string, std::string> &, std::map<std::string, std::string> &);
        bool    rstr_Rename                 (std::map<std::string, std::string> &, std::map<std::string, std::string> &, int &cnt);

        bool        ren                     (std::string, std::string, std::string &, std::string &, size_t = 0u, bool = true);
        std::string getNumericName          (size_t, size_t);
        std::string toLower                 (std::string) const;
        std::string getUpperLevelDirectory  (const std::string &, size_t) const;
        std::string getCurrentDirFromPath   (const std::string &) const;

	private:

        char        _mode;

		std::string _dir;
		std::string _dirResize;
		std::string _dirQuality;
        std::string _dirResized;
        std::string _dirSkipped;

        std::map<std::string, dirInfo> _mapDirs;

        float  _threshold_dimension;
        float  _threshold_size;
        float  _threshold_quality;
        size_t _threshold_resize;
        size_t _threshold_cover;
};
