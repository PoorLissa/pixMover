#include "myApp.h"

// -----------------------------------------------------------------------------------------------

namespace myGlobals {

    const char* tab_of_four         = "    ";
    const char* ren_to              = " => ";
    const char* ren_ok              = "  -- Ok";
    const char* ren_fail            = "  -- Fail";
    const char* aux_dir_resize      = "[__do_resize]";
    const char* aux_dir_quality     = "[__do_lower_quality]";
    const char* aux_dir_resized     = "[__files.r]";
    const char* aux_dir_skipped     = "[__skipped]";
    const char* aux_dir_not_resized = "[-not-resized-]";
};

// -----------------------------------------------------------------------------------------------

void myApp::selectUserOption()
{
	char ch(0);

	std::cout << std::endl;
	std::cout << " Select mode:\n\n";
	std::cout << " 1. Rearrange directories depending on files size/dimensions (Before the resize)\n";
	std::cout << " 2. Analyze the files (After the resize)\n";
	std::cout << " 3. Restore original file names\n";
	std::cout << " q. Exit\n\n";
	std::cout << " >> ";

	std::cin >> ch;

	switch (ch)
	{
		case '1':
			_mode = MODE::REARRANGE;
			break;

		case '2':
			_mode = MODE::ANALYZE;
			break;

		case '3':
			_mode = MODE::RESTORE;
			break;

        default:
            _mode = MODE::EXIT;
	}

	return;
}

// -----------------------------------------------------------------------------------------------

void myApp::process()
{
    selectUserOption();

    std::cout << " Selected mode: ";

	switch (_mode)
	{
		case MODE::REARRANGE: {

                std::cout << "Rearranging\n" << std::endl;

				// Read file structure and get files info
				std::cout << " Reading directory structure..." << std::endl;
				readFiles(_dir);

				if (_mapDirs.size())
				{
					// Rename files (optional, will be asked about it)
					std::cout << " Renaming files..." << std::endl;
					renameFiles();

					// Move directories
					std::cout << " Rearranging directories..." << std::endl;
					sortDirs();
				}

				std::cout << " Done: OK" << std::endl;

			}
			break;

		case MODE::ANALYZE: {

                std::cout << "Analysing\n" << std::endl;

                std::cout << myGlobals::tab_of_four << "Looking for files only in these subfolders:" << std::endl;
                std::cout << myGlobals::tab_of_four << "  " << myGlobals::aux_dir_resize  << std::endl;
                std::cout << myGlobals::tab_of_four << "  " << myGlobals::aux_dir_quality << std::endl;
                std::cout << std::endl;

				createAuxDirs(false);
				std::cout << " Checking files...\n" << std::endl;
				checkOnRenamed();

			}
			break;

		case MODE::RESTORE: {

                std::cout << "Restoring\n" << std::endl;
                restoreFileNames();

			}
			break;

		default:
			std::cout << "Exiting" << std::endl;
	}

	return;
}

// -----------------------------------------------------------------------------------------------

// Get list of directories within the directory
void myApp::findDirs(std::string dir)
{
    const char* names_to_skip[] = { ".", "..",
                                        myGlobals::aux_dir_quality,
                                        myGlobals::aux_dir_resize,
                                        myGlobals::aux_dir_resized,
                                        myGlobals::aux_dir_skipped
    };

	auto isDirOk = [&names_to_skip](const std::string &name) -> bool
	{
		for (size_t i = 0; i < sizeof(names_to_skip) / sizeof(names_to_skip[0]); i++)
			if (name == names_to_skip[i])
				return false;

		return true;
	};

	WIN32_FIND_DATA ffd;
    TCHAR szDir[MAX_PATH] = { 0 };
	HANDLE hFind = INVALID_HANDLE_VALUE;

	bool error(false);
	_mapDirs.clear();
	std::string Dir = dir + "*.*";

	for (size_t i = 0; i < Dir.length(); i++)
	{
		szDir[i + 0] = Dir[i];
		szDir[i + 1] = '\0';
	}

	hFind = FindFirstFile(szDir, &ffd);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do {

			if ( ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
			{
				size_t i = 0;
				std::string name(""), fullName(dir);

				while (ffd.cFileName[i] != '\0')
				{
					char ch = static_cast<char>(ffd.cFileName[i]);

					name.push_back(ch);
					fullName.push_back(ch);
					i++;
				}

				fullName += "\\";

				// Remember the name
                if (isDirOk(name))
				{
					if (fullName != _dirQuality && fullName != _dirResize)
					{
						dirInfo di;
						di.fullName = fullName;
						di.mode		= 0;

						_mapDirs[name] = di;
					}
				}
			}

		} while (FindNextFile(hFind, &ffd));

		FindClose(hFind);
	}

	return;
}

// -----------------------------------------------------------------------------------------------

// Get list of files within the directory
void myApp::findFiles(std::string dir, dirInfo &di)
{
	const char* validExtensions[] = { ".jpg", ".jpeg" };

	WIN32_FIND_DATA ffd;
    TCHAR szDir[MAX_PATH] = { 0 };
	HANDLE hFind = INVALID_HANDLE_VALUE;

	bool error(false);
	size_t size(0);
	di.files.clear();
	std::string Dir = dir + "*.*";
	struct _stat32 buf;

	for (size_t i = 0; i < Dir.length(); i++)
	{
		szDir[i+0] = Dir[i];
		szDir[i+1] = '\0';
	}

	hFind = FindFirstFile(szDir, &ffd);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do {

			size_t i(0u);
			std::string fName;

			while (ffd.cFileName[i] != '\0')
			{
				char ch = static_cast<char>(ffd.cFileName[i++]);
				fName.push_back(ch);
			}

			if ((ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				// Found directory
				if (fName != "." && fName != "..")
					di.mode = 1;
			}
			else
			{
				size_t i = 0;
				std::string name(""), fullName(dir);

				while (ffd.cFileName[i] != '\0')
				{
					char ch = static_cast<char>(ffd.cFileName[i]);

					name.push_back(ch);
					fullName.push_back(ch);
					i++;
				}

				bool fileOk = false;

				// Check file's extension:
				for (int i = 0; i < sizeof(validExtensions) / sizeof(validExtensions[0]); i++)
				{
					size_t pos = name.find_last_of('.');

					if (pos != std::string::npos)
					{
						if (name.substr(pos, name.length()) == validExtensions[i])
						{
							fileOk = true;
							break;
						}
					}
				}

				if (fileOk)
				{
					// Get data associated with "crt_stat.c":
					if (_stat32(fullName.c_str(), &buf))
					{
						size = 0u;
					}
					else
					{
						size = buf.st_size;
					}

					size_t pos = name.find_last_of('.');

					// Remember the name
					fileInfo inf;
					inf.fullName  = fullName;
					inf.size	  = size;
					inf.canRename = true;
					inf.isResized = (pos >= 2u && name.substr(pos - 2u, 3u) == ".r.");

					di.files.emplace(name, inf);
				}
			}

		} while (FindNextFile(hFind, &ffd));

		FindClose(hFind);
	}

	return;
}

// -----------------------------------------------------------------------------------------------

// Gets image dimensions x and y
bool myApp::GetImageSize(const char* fileName, int& x, int& y)
{
    bool res = false;
    FILE* f = nullptr;

    auto getJpg = [](bool& res, unsigned char* buf, int& x, int& y)
    {
        if (buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF)
        {
            y = (buf[7] << 8) + buf[8];
            x = (buf[9] << 8) + buf[10];
            res = true;
        }
    };

    auto getGif = [](bool& res, unsigned char* buf, int& x, int& y)
    {
        if (buf[0] == 'G' && buf[1] == 'I' && buf[2] == 'F')
        {
            x = buf[6] + (buf[7] << 8);
            y = buf[8] + (buf[9] << 8);
            res = true;
        }
    };

    auto getPng = [](bool& res, unsigned char* buf, int& x, int& y)
    {
        if (buf[0] == 0x89 && buf[1] == 'P' && buf[2] == 'N' && buf[3] == 'G' && buf[4] == 0x0D && buf[5] == 0x0A && buf[6] == 0x1A && buf[7] == 0x0A
            && buf[12] == 'I' && buf[13] == 'H' && buf[14] == 'D' && buf[15] == 'R')
        {
            x = (buf[16] << 24) + (buf[17] << 16) + (buf[18] << 8) + (buf[19] << 0);
            y = (buf[20] << 24) + (buf[21] << 16) + (buf[22] << 8) + (buf[23] << 0);
            res = true;
        }
    };



    if (!fopen_s(&f, fileName, "rb"))
    {
        // Check file's length
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (len >= 24)
        {
            // Strategy:
            // reading GIF dimensions requires the first 10 bytes of the file
            // reading PNG dimensions requires the first 24 bytes of the file
            // reading JPEG dimensions requires scanning through jpeg chunks
            // In all formats, the file is at least 24 bytes big, so we'll read that always
            unsigned char buf[24];
            fread(buf, 1, 24, f);

            // For JPEGs, we need to read the first 12 bytes of each chunk.
            // We'll read those 12 bytes at buf+2...buf+14, i.e. overwriting the existing buf.

/*
			// Original 
			bool jpg1 = buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF && buf[3] == 0xE0 && buf[6] == 'J' && buf[7] == 'F' && buf[8] == 'I' && buf[9] == 'F';

			// Found this later
			bool jpg2 = buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF && buf[3] == 226 && buf[5] == 'X' && buf[6] == 'I' && buf[7] == 'C' && buf[8] == 'C';

            if (jpg1 || jpg2)
*/
			// Skip this check for a while, as we encounter lots of different patterns here for different files
            {
                long pos = 2;

                while (buf[2] == 0xFF)
                {
                    if (buf[3] == 0xC0 || buf[3] == 0xC1 || buf[3] == 0xC2 || buf[3] == 0xC3 || buf[3] == 0xC9 || buf[3] == 0xCA || buf[3] == 0xCB)
                        break;

                    pos += 2 + (buf[4] << 8) + buf[5];

                    if (pos + 12 > len)
                        break;

                    fseek(f, pos, SEEK_SET);
                    fread(buf + 2, 1, 12, f);
                }
            }

            // JPEG: (first two bytes of buf are first two bytes of the jpeg file; rest of buf is the DCT frame
            if (!res)
            {
                getJpg(res, buf, x, y);
            }

            // GIF: first three bytes say "GIF", next three give version number. Then dimensions
            if (!res)
            {
                getGif(res, buf, x, y);
            }

            // PNG: the first frame is by definition an IHDR frame, which gives dimensions
            if (!res)
            {
                getPng(res, buf, x, y);
            }
        }

        fclose(f);
    }

    return res;
}

// -----------------------------------------------------------------------------------------------

bool myApp::dirExists(std::string dir)
{
    // Check if path is relative or absolute
    if (dir.length() > 3u && dir[1] != ':')
        dir = _dir + dir;

	DWORD dwAttrib = GetFileAttributesA(dir.c_str());

	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
};

// -----------------------------------------------------------------------------------------------

bool myApp::fileExists(std::string file)
{
    DWORD dwAttrib = GetFileAttributesA(file.c_str());

    return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
};

// -----------------------------------------------------------------------------------------------

void myApp::mkDir(std::string dir)
{
    // Check if path is relative or absolute
    if(dir.length() > 3u && dir[1] != ':')
	    dir = _dir + dir;

	CreateDirectoryA(dir.c_str(), NULL);

	return;
}

// -----------------------------------------------------------------------------------------------

void myApp::mvDir(std::string shortName, std::string fullName, enum class DIRS d)
{
	std::string dest(_dir);

    static char bits[4] = { 0 };

	switch (d)
	{
		case DIRS::RESIZE:
			dest = _dirResize;
			break;

		case DIRS::QUALITY:
			dest = _dirQuality;
			break;

		case DIRS::RESIZED:
			dest = _dirResized;
			break;

		case DIRS::SKIPPED:
			dest = _dirSkipped;
			break;
	}

    int D = static_cast<int>(d);

    if (bits[D] == 0)
    {
        bits[D] = 1;

        if (!dirExists(dest))
        {
            std::cout << myGlobals::tab_of_four << "Creating dir : " << dest << std::endl;
            mkDir(dest);
        }
    }

	dest += shortName;

    BOOL b = MoveFileExA(fullName.c_str(), dest.c_str(), MOVEFILE_WRITE_THROUGH);

    if (!b)
    {
        std::cout << myGlobals::tab_of_four << "Move func failed. See error log for details." << std::endl;
        std::string err = " --> Move failed: " + fullName + " => " + dest;
        logError(err);
    }

	return;
}

// -----------------------------------------------------------------------------------------------

// Move files to two separate directories
void myApp::mvFiles(std::string shortName, std::string fullName, std::map<std::string, myApp::fileInfo, myCmp> &files)
{
    std::string dest1(_dirQuality);
    std::string dest2(_dirResize);

    if (!dirExists(dest1))
    {
        std::cout << myGlobals::tab_of_four << "Creating dir : " << dest1 << std::endl;
        mkDir(dest1);
    }

    if (!dirExists(dest2))
    {
        std::cout << myGlobals::tab_of_four << "Creating dir : " << dest2 << std::endl;
        mkDir(dest2);
    }

    for (auto it = files.begin(); it != files.end(); ++it)
    {
        std::string src = fullName + it->first, dest = "";

        switch (it->second.action)
        {
            case 0:
                break;

            // Quality
            case 1:
                {
                    dest = dest1 + shortName;

                    if (!dirExists(dest))
                    {
                        std::cout << myGlobals::tab_of_four << "Creating dir : " << dest << std::endl;
                        mkDir(dest);
                    }
                    
                    dest += "\\" + it->first;
                    BOOL bbb = MoveFileExA(src.c_str(), dest.c_str(), MOVEFILE_WRITE_THROUGH);

                    if (!bbb)
                    {
                        std::cout << myGlobals::tab_of_four << "Move func failed. See error log for details." << std::endl;
                        std::string err = " --> Move failed: " + fullName + " => " + dest;
                        logError(err);
                    }
                }
                break;

            // Resize
            case 2:
                {
                    dest = dest2 + shortName;

                    if (!dirExists(dest))
                    {
                        std::cout << myGlobals::tab_of_four << "Creating dir : " << dest << std::endl;
                        mkDir(dest);
                    }
                    
                    dest += "\\" + it->first;
                    BOOL bbb = MoveFileExA(src.c_str(), dest.c_str(), MOVEFILE_WRITE_THROUGH);

                    if (!bbb)
                    {
                        std::cout << myGlobals::tab_of_four << "Move func failed. See error log for details." << std::endl;
                        std::string err = " --> Move failed: " + fullName + " => " + dest;
                        logError(err);
                    }
                }
                break;
        }
    }

    return;
}

// -----------------------------------------------------------------------------------------------

void myApp::createAuxDirs(bool doCreate)
{
	if (doCreate)
	{
		if (!dirExists(myGlobals::aux_dir_resize))
			mkDir(myGlobals::aux_dir_resize);

		if (!dirExists(myGlobals::aux_dir_quality))
			mkDir(myGlobals::aux_dir_quality);

		if (!dirExists(myGlobals::aux_dir_resized))
			mkDir(myGlobals::aux_dir_resized);

		if (!dirExists(myGlobals::aux_dir_skipped))
			mkDir(myGlobals::aux_dir_skipped);
	}

	_dirResize  = _dir + myGlobals::aux_dir_resize  + "\\";
	_dirQuality = _dir + myGlobals::aux_dir_quality + "\\";
	_dirResized = _dir + myGlobals::aux_dir_resized + "\\";
	_dirSkipped = _dir + myGlobals::aux_dir_skipped + "\\";

	return;
}

// -----------------------------------------------------------------------------------------------

bool myApp::ren(std::string oldName, std::string newName, std::string &newFullName, std::string &logData, size_t cnt /*=0*/, bool ext /*=true*/)
{
	size_t pos = oldName.find_last_of('\\') + 1;

	// add new name
	newFullName = oldName.substr(0, pos) + newName;

	// add counter
	if (cnt)
		newFullName += std::to_string(cnt);

	// add extension
	if (ext)
	{
		pos = oldName.find_last_of('.');

		if (pos != std::string::npos)
			newFullName += oldName.substr(pos, oldName.length());
	}

	if (oldName != newFullName)
	{
//		std::cout << " ren '" << oldName << "' to '" << newFullName << "'" << std::endl;

		BOOL b = MoveFileA(oldName.c_str(), newFullName.c_str());

		logData += oldName;
        logData += myGlobals::ren_to;
		logData += newFullName.substr(newFullName.find_last_of('\\') + 1u, newFullName.length());
		logData += b ? myGlobals::ren_ok : myGlobals::ren_fail;
		logData += "\n";

		return b != 0;
	}

	return false;
}

// -----------------------------------------------------------------------------------------------

std::string myApp::getNumericName(size_t num, size_t len)
{
	std::string res = std::to_string(num);

	len = len < 3u ? 3u : len;

	while (res.length() != len)
		res.insert(0, "0");

	return res;
}

// -----------------------------------------------------------------------------------------------

void myApp::renameFiles()
{
	static char		opt_RenameFiles(0);
	static char		opt_RenameCovers(0);
	std::string		newFullName;
	std::string		logData;

	if (!opt_RenameFiles)
	{
		std::cout << " Do you want to rename covers? (y/n): ";
		std::cin >> opt_RenameCovers;
		std::cout << std::endl;
	}

	if (!opt_RenameFiles)
	{
		std::cout << " Do you want to rename files in numeric order? (y/n): ";
		std::cin >> opt_RenameFiles;
		std::cout << std::endl;
	}

	bool doRenameCovers = (opt_RenameCovers == 'y' || opt_RenameCovers == 'Y');
	bool doRenameFiles  = (opt_RenameFiles  == 'y' || opt_RenameFiles  == 'Y');

	// For each dir: rename files
	for (auto iter = _mapDirs.begin(); iter != _mapDirs.end(); ++iter)
	{
		renFiles(iter->second, doRenameCovers, doRenameFiles, logData);
	}

	// Save the log
	std::fstream file;
	file.open(_dir + "\\[info].txt", std::fstream::in | std::fstream::app);

	if (file.is_open())
	{
		file.write(logData.c_str(), logData.length());
		file.close();
	}

	return;
}

// -----------------------------------------------------------------------------------------------

void myApp::renFiles(dirInfo &di, bool doRenameCovers, bool doRenameFiles, std::string &logData)
{
	bool			resizedFound = false;
	std::string		newFullName;
	size_t			cnt_cover(0u), cnt_z_cover(0u), cnt(0u), totalSize(0u);
	const char *	d = "==========================================================================================\n";

	// Skip any folder that has subfolders
	if (di.mode == 1)
		return;

	// Try to find out if any of the files were resized already
    // Also, gather some additional statistics
	for (auto it = di.files.begin(); it != di.files.end(); ++it)
	{
        // For the avg. file size
		totalSize  += it->second.size;

		if (it->second.isResized)
			resizedFound = true;
	}

	// No need to rename anything: some files have already been renamed earlier
	if (resizedFound)
		return;

	logData += d;

	// Try to find covers:
	for (auto it = di.files.begin(); it != di.files.end(); ++it)
	{
        // Try to find covers by name:
		if (it->second.canRename)
		{
			std::string name = toLower(it->first);

			// Cover found
			if (name.find("cover") != std::string::npos)
			{
				// Mark it, so it would not be renamed later
				it->second.canRename = false;

				if (doRenameCovers)
				{
					if (name.find("z_cover") == 0u)
					{
						cnt_z_cover++;
						continue;
					}

					if (name.find("_cover") == 0u)
					{
						cnt_cover++;
						continue;
					}

					if (name.find("clean") != std::string::npos)
					{
						ren(it->second.fullName, "z_cover", newFullName, logData, cnt_z_cover);
						cnt_z_cover++;
						continue;
					}

					{
						ren(it->second.fullName, "_cover", newFullName, logData, cnt_cover);
						cnt_cover++;
                        continue;
					}
				}
			}
		}

        // Try to find files that possibly might be covers:
        if (doRenameCovers)
        {
            // Try to find files that are significantly smaller (with respect to file dimensions)
            if (it->second.canRename)
            {
                int w = it->second.width > it->second.height ? it->second.width  : it->second.height;
                int h = it->second.width > it->second.height ? it->second.height : it->second.width;

                // File's max dimension has to be less than [_threshold_cover]
#if 0
                // Average
                if (_threshold_cover > 0u && w <= _threshold_cover)
                {
                    // File's dimensions have to be less than average dimension in this folder
                    if (di.avg_height / h > _threshold_dimension && di.avg_width / w > _threshold_dimension)
                    {
//                      std::cout << it->first << " [" << w << "x" << h << "]" << std::endl;

                        // Rename 'aaa.jpg' => '_cover;dimension;aaa.jpg'
                        ren(it->second.fullName, "_cover;dimension.avg;" + it->first, newFullName, logData, 0u, false);
                        it->second.canRename = false;
                        cnt_cover++;
                    }
                }
#else
                // Median
                if (_threshold_cover > 0u && w <= _threshold_cover)
                {
                    // File's dimensions have to be less than median dimension in this folder
                    if (di.median_height / h > _threshold_dimension || di.median_width / w > _threshold_dimension)
                    {
//                      std::cout << it->first << " [" << w << "x" << h << "]" << std::endl;

                        // Rename 'aaa.jpg' => '_cover;dimension;aaa.jpg'
                        ren(it->second.fullName, "_cover;dimension.mdn;" + it->first, newFullName, logData, 0u, false);
                        it->second.canRename = false;
                        cnt_cover++;
                    }
                }
#endif
            }

            // Try to find files that are significantly smaller (with respect to file size)
            if (it->second.canRename)
            {
                if (di.avg_size / it->second.size > _threshold_size)
                {
                    // Rename 'aaa.jpg' => '_cover;size;aaa.jpg'
                    ren(it->second.fullName, "_cover;size;" + it->first, newFullName, logData, 0u, false);
                    it->second.canRename = false;
                    cnt_cover++;
                }
            }
        }
	}

	// Rename the rest of the files:
    if (doRenameFiles)
	{
		std::vector<std::string> vec;

		// Make sure we have enough length for numeric name ('001', '0001', '00001', etc)
		size_t totalCnt = di.files.size() - cnt_cover - cnt_z_cover;
		totalCnt = std::to_string(totalCnt).length();

		// Pass 1, to temporary name:
		for (auto it = di.files.begin(); it != di.files.end(); ++it)
		{
			if (it->second.canRename && !it->second.isResized)
			{
                std::string tmpName("_tmp_" + getNumericName(++cnt, totalCnt));

				ren(it->second.fullName, tmpName, newFullName, logData);
				it->second.canRename = false;

				vec.push_back(newFullName);
			}
		}

		cnt = 0u;

		// Pass 2, final:
		for (auto it = vec.begin(); it != vec.end(); ++it)
		{
			ren(*it, getNumericName(++cnt, totalCnt), newFullName, logData);
		}
	}

	return;
}

// -----------------------------------------------------------------------------------------------

// Read info about all the files into map
void myApp::readFiles(std::string dir)
{
	int x(0), y(0), i(0);

	findDirs(dir);

	std::cout << " Found " << _mapDirs.size() << " subfolder(s):\n" << std::endl;

	// For each dir:
	for (auto iter = _mapDirs.begin(); iter != _mapDirs.end(); ++iter)
	{
        size_t totalSize(0u), totalWidth(0u), totalHeight(0u);

        std::cout << myGlobals::tab_of_four << ++i << ": " << iter->second.fullName << std::endl;

		// Get all files in this directory
		findFiles(iter->second.fullName, iter->second);

		switch (iter->second.mode)
		{
			case 0: {

                    std::cout << "\t--> Contains " << iter->second.files.size() << " file(s)" << std::endl;

                    // We'll use lists to get median values
                    std::list<size_t>   l1;
                    std::list<int>      l2, l3;

					// For each file:
					for (auto it = iter->second.files.begin(); it != iter->second.files.end(); ++it)
					{
						GetImageSize(it->second.fullName.c_str(), x, y);

						it->second.width  = x;
						it->second.height = y;

                        // For the avg. file size and dimensions
                        totalSize   += it->second.size;
                        totalWidth  += x > y ? x : y;
                        totalHeight += x > y ? y : x;

                        l1.emplace_back(it->second.size);
                        l2.emplace_back(x > y ? x : y);
                        l3.emplace_back(x > y ? y : x);

#if defined _DEBUG
                        std::cout << myGlobals::tab_of_four << myGlobals::tab_of_four << myGlobals::tab_of_four << it->first << " [" << x << "x" << y << "]" << std::endl;
#endif
					}

                    if (iter->second.files.size())
                    {
                        iter->second.avg_size   = float(totalSize  ) / iter->second.files.size();
                        iter->second.avg_width  = float(totalWidth ) / iter->second.files.size();
                        iter->second.avg_height = float(totalHeight) / iter->second.files.size();

                        // Get median values
                        {
                            l1.sort();
                            l2.sort();
                            l3.sort();

                            auto it1 = l1.begin();
                            auto it2 = l2.begin();
                            auto it3 = l3.begin();

                            for (size_t i = 0; i < l1.size() / 2u; i++)
                            {
                                ++it1;
                                ++it2;
                                ++it3;
                            }

                            iter->second.median_size   = *it1;
                            iter->second.median_width  = *it2;
                            iter->second.median_height = *it3;
                        }
                    }

#if defined _DEBUG
                    std::cout << myGlobals::tab_of_four << myGlobals::tab_of_four << " --> size   : " << iter->second.avg_size   << " (avg), " << iter->second.median_size   << " (median)" << std::endl;
                    std::cout << myGlobals::tab_of_four << myGlobals::tab_of_four << " --> width  : " << iter->second.avg_width  << " (avg), " << iter->second.median_width  << " (median)" << std::endl;
                    std::cout << myGlobals::tab_of_four << myGlobals::tab_of_four << " --> height : " << iter->second.avg_height << " (avg), " << iter->second.median_height << " (median)" << std::endl;
                    std::cout << std::endl;
#endif
				}
				break;

			case 1: {

					std::cout << "\t--> Has subfolders. Will be skipped" << std::endl;

				}
				break;
		}
	}

	std::cout << std::endl;

	return;
}

// -----------------------------------------------------------------------------------------------

void myApp::sortDirs()
{
    createAuxDirs(false);

	// For each dir:
	for (auto iter = _mapDirs.begin(); iter != _mapDirs.end(); ++iter)
	{
		auto mapFiles = &(iter->second.files);

		int maxX(0), maxY(0), x(0), y(0);
		size_t SizeTotal(0u), SizeQuality(0u), SizeResize(0u), cntResize(0u), cntQuality(0u), cntResized(0u);
		bool doSkip(false);
		float avg = 0.0f;

		if (iter->second.mode == 0)
		{
			// For each file:
			for (auto it = mapFiles->begin(); it != mapFiles->end(); ++it)
			{
                it->second.action = 0;

				size_t* currentSize = &(it->second.size);

				SizeTotal += *currentSize;

				if (it->second.isResized)
				{
					cntResized++;
				}
				else
				{
					// Check if the file is larger than 1 Mb:
					if (*currentSize >= 1024 * 1024)
					{
						SizeQuality += *currentSize;
						cntQuality++;
                        it->second.action = 1;
					}

					x = it->second.width;
					y = it->second.height;

					if (x > maxX)
					{
						maxX = x;
					}

					if (y > maxY)
					{
						maxY = y;
					}

					int minSize = x > y ? y : x;

					if (minSize > _threshold_resize)
					{
						SizeResize += *currentSize;
						cntResize++;
                        it->second.action = 2;
					}
				}
			}
		}
		else
		{
			if (iter->second.mode == 1)
				doSkip = true;
		}

		std::cout << "\n" << myGlobals::tab_of_four << "Total Files (" << mapFiles->size() << "):" << std::endl;

		if (mapFiles->size())
		{
			float totalSize = (float)SizeTotal / (1024 * 1024);
			float avgSize = totalSize / mapFiles->size();

			std::cout << myGlobals::tab_of_four << "Total File Size = " << totalSize << " Mb (" << SizeTotal << ") bytes" << std::endl;
			std::cout << myGlobals::tab_of_four << "Avrg. File Size = " << avgSize << " Mb (" << SizeTotal / mapFiles->size() << ") bytes" << std::endl;
		}

		std::cout << "\n" << myGlobals::tab_of_four << "Files to Resize (" << cntResize << "):" << std::endl;

		if (cntResize)
		{
			float totalSize = (float)SizeResize / (1024 * 1024);
			float avgSize = totalSize / cntResize;

			std::cout << myGlobals::tab_of_four << "Total File Size = " << totalSize << " Mb (" << SizeResize << ") bytes" << std::endl;
			std::cout << myGlobals::tab_of_four << "Avrg. File Size = " << avgSize << " Mb (" << SizeResize / cntResize << ") bytes" << std::endl;
		}

		std::cout << "\n "<< myGlobals::tab_of_four << "Files Over 1 Mb (" << cntQuality << "):" << std::endl;

		if (cntQuality)
		{
			float totalSize = (float)SizeQuality / (1024 * 1024);
			avg = totalSize / cntQuality;

			std::cout << myGlobals::tab_of_four << "Total File Size = " << totalSize << " Mb (" << SizeQuality << ") bytes" << std::endl;
			std::cout << myGlobals::tab_of_four << "Avrg. File Size = " << avg << " Mb (" << SizeQuality / cntQuality << ") bytes" << std::endl;
		}

		if (doSkip)
		{
			std::cout << myGlobals::tab_of_four << "Folder will be skipped" << std::endl;
		}

		std::cout << std::endl;

		do
		{
            // Skip this folder
			if (doSkip)
			{
				mvDir(iter->first, iter->second.fullName, myApp::DIRS::SKIPPED);
				break;
			}

            // Already resized
			if (cntResized)
			{
				mvDir(iter->first, iter->second.fullName, myApp::DIRS::RESIZED);
				break;
			}

            if (cntResize && avg > _threshold_quality)
            {
                mvFiles(iter->first, iter->second.fullName, *mapFiles);
                break;
            }

            // Resize every file
			if (cntResize)
			{
				mvDir(iter->first, iter->second.fullName, myApp::DIRS::RESIZE);
				break;
			}

            // Reduce quality for every file
			if (avg > _threshold_quality)
			{
				mvDir(iter->first, iter->second.fullName, myApp::DIRS::QUALITY);
				break;
			}

		} while (false);
	}

	return;
}

// -----------------------------------------------------------------------------------------------

void myApp::checkOnRenamed()
{
	auto isRenamed = [](std::string name) -> bool
	{
		size_t pos = name.find_last_of('.');

		if (pos != std::string::npos && pos > 2u)
			return name.substr(pos - 2u, 3u) == ".r.";

		return false;
	};

	auto isCover = [](std::string name) -> bool
	{
		return name.find("cover") != std::string::npos;
	};

	// ------------------------------------------------------------------------

	std::vector<std::string *> vec;

	vec.emplace_back(&_dirQuality);
	vec.emplace_back(&_dirResize);

	// Read file structure and get files info
	// Upper level directories:
	for (size_t i = 0; i < vec.size(); i++)
	{
        std::cout << " Reading files in " << *vec[i] << std::endl;

		readFiles(*vec[i]);

		// Inner directories
		for (auto iter = _mapDirs.begin(); iter != _mapDirs.end(); ++iter)
		{
			auto mapFiles = &(iter->second.files);

			std::map<std::string, myApp::fileInfo *> m;
			std::map<std::string, std::string> map_not_resized;
			size_t cnt_original(0u), cnt_resized(0u), size_original(0u), size_resized(0u);

			for (auto it = mapFiles->begin(); it != mapFiles->end(); ++it)
				m[it->first] = &(it->second);

			// Files
			for (auto it = m.begin(); it != m.end(); ++it)
			{
				// Leave covers as they are
				if (isCover(it->first))
					continue;

				if (!isRenamed(it->first))
				{
					cnt_original++;

					std::string str = it->first.substr(0, it->first.find_last_of('.')) + ".r.jpg";

					// This original file has a '.r.jpg' sibling (has been resized)
					if (m.count(str))
					{
						cnt_resized++;

						size_original += it->second->size;
						size_resized  += m[str]->size;
					}
					else
					{
						map_not_resized[it->first] = it->second->fullName;
					}
				}
			}


			std::string data = " - Files NOT resized (" + std::to_string(map_not_resized.size()) + ") -\n";

			for (auto it = map_not_resized.begin(); it != map_not_resized.end(); ++it)
			{
				data += "\t";
				data += it->first;
				data += '\n';
			}


			data += " =======================================================\n";
			data += " - Files resized (" + std::to_string(cnt_resized) + ") -\n";
			data += "  Original size\t\t:\t" + std::to_string((float)size_original / 1024 / 1024);
			data += '\n';
			data += "  Resized size\t\t:\t" + std::to_string((float)size_resized  / 1024 / 1024);
			data += '\n';
			data += "  Saved disk space\t:\t" + std::to_string(((float)size_original / 1024 / 1024) - ((float)size_resized / 1024 / 1024));
			data += '\n';

			if (size_original <= size_resized)
				data += "\n  Be careful! Resized files are LARGER! 0_o\n";


			// Move files into another directory
			if (map_not_resized.size() > 4u)
			{
				std::string dir = iter->second.fullName + myGlobals::aux_dir_not_resized;
                std::string dir_not_resized(myGlobals::aux_dir_not_resized);
                dir_not_resized += "\\";

				CreateDirectoryA(dir.c_str(), NULL);

				for (auto it = map_not_resized.begin(); it != map_not_resized.end(); ++it)
				{
					std::string src = it->second;
					std::string dst = it->second;

					dst.insert(dst.find_last_of('\\') + 1u, dir_not_resized);

					MoveFileExA(src.c_str(), dst.c_str(), MOVEFILE_WRITE_THROUGH);
				}
			}

			// Save log info into a file. If the file already exists, it will be overwritten
			std::fstream file;
			file.open(iter->second.fullName + "\\[info].txt", std::fstream::out);

			if (file.is_open())
			{
				file.write(data.c_str(), data.length());
				file.close();
			}
		}
	}

	(void)getchar();

	return;
}

// -----------------------------------------------------------------------------------------------

std::string myApp::toLower(std::string str) const
{
	std::string res(str);

	for (size_t i = 0; i < res.length(); i++)
	{
		char ch = res[i];

		if (ch > 64 && ch < 91)
			ch += 32;

		res[i] = ch;
	}

	return res;
}

// -----------------------------------------------------------------------------------------------

void myApp::restoreFileNames()
{
    std::vector<std::string> vec;

    int res = rstr_CheckFileStructure(vec);

    std::cout << " Checking status: ";

    if (!res)
    {
        std::cout << "Fail\n" << std::endl;
    }
    else
    {
        std::map<std::string, std::string> m1, m2;

        std::cout << "Success\n" << std::endl;
        std::cout << " Moving on...\n" << std::endl;

        bool parseOk = rstr_ParseInfoVec(vec, m1, m2);

        if (!parseOk)
        {
            std::cout << " Parsing '[info].txt' failed" << std::endl;
            std::cout << myGlobals::tab_of_four << "Possible reasons:" << std::endl;
            std::cout << myGlobals::tab_of_four << " - One of the lines in the file has status 'Fail'" << std::endl;
            std::cout << myGlobals::tab_of_four << " - Incorrect syntax within one of the lines" << std::endl;
            std::cout << std::endl;
        }
        else
        {
            std::cout << " Parsing results:\n";

            for (auto m : m1)
                std::cout << myGlobals::tab_of_four << "[" << m.first << "] => [" << m.second << "]" << std::endl;

            std::cout << "\n";
            for (auto m : m2)
                std::cout << myGlobals::tab_of_four << "[" << m.first << "] => [" << m.second << "]" << std::endl;

            std::cout << "\n Renaming files:\n";

            int cnt = 0;

            if (rstr_Rename(m2, m1, cnt))
            {
                std::cout << "\n Overall Renaming Status: Ok";
            }
            else
            {
                std::cout << "\n Overall Renaming Status: Fail";
            }

            std::cout << "\n\n";
        }
    }

    return;
}

// -----------------------------------------------------------------------------------------------

int myApp::rstr_CheckFileStructure(std::vector<std::string> &vec)
{
    auto check_vec = [](std::vector<std::string>& vec) -> bool
    {
        bool delimFound = false;

        for (auto iter = vec.begin(); iter != vec.end(); ++iter)
        {
            if ((*iter)[0] == '=')
                delimFound = true;

            if (delimFound && (*iter)[0] != '=')
                return false;
        }

        // So far so good. Let's remove delim lines
        while (vec.back()[0] == '=')
            vec.pop_back();

        return true;
    };

	// ------------------------------------------------------------

    std::cout << " Checking file structure... " << std::endl;

    // Here we're going to look for the [info].txt file in 2 subsequent directories starting with the current one
    // But if the 2nd level directory is one of our AUX directories, we'll also try to find the file 3 levels up from here
    std::string path = _dir;

    for (size_t i = 0; i < 3u; i++)
    {
        std::string dir = getUpperLevelDirectory(path, i);

        if (dirExists(dir))
        {
            std::string info = dir + "[info].txt";

            if (fileExists(info))
            {
                std::cout << myGlobals::tab_of_four << "[info].txt is found in '" << dir << "'" << std::endl;

                if (rstr_GetHistory(vec, info))
                {
                    std::cout << myGlobals::tab_of_four << "[info].txt contains some records in it" << std::endl;

                    if (check_vec(vec))
                    {
                        std::cout << myGlobals::tab_of_four << "[info].txt records look good" << std::endl;
                        return 1;
                    }
                    else
                    {
                        std::cout << myGlobals::tab_of_four << "[info].txt records refer to more than one distinct directory" << std::endl;
                    }
                }
            }

            if (i == 1u)
            {
                std::string curr = getCurrentDirFromPath(dir);

                bool isAUX = curr == myGlobals::aux_dir_quality ||
                             curr == myGlobals::aux_dir_resize  ||
                             curr == myGlobals::aux_dir_resized ||
                             curr == myGlobals::aux_dir_skipped;
                if(!isAUX)
                    break;
            }
        }
    }

    return 0;
}

// -----------------------------------------------------------------------------------------------

bool myApp::rstr_GetHistory(std::vector<std::string>& vec, std::string &info)
{
    bool infoFound = false;
    std::fstream file;

    file.open(info, std::fstream::in);

    if (file.is_open())
    {
        info.clear();
        vec.clear();
        std::string line, dir(_dir);

        // 'C:\dir1\dir2\001' => '001'
        if (dir.back() == '\\')
        {
            size_t pos = dir.rfind('\\', dir.length()-2u) + 1u;
            dir = dir.substr(pos, dir.length() - pos - 1u);
        }
        else
        {
            size_t pos = dir.find_last_of('\\') + 1u;
            dir = dir.substr(pos, dir.length() - pos);
        }

        dir = "\\" + dir + "\\";

        while (std::getline(file, line))
        {
            if (line[0] == '=')
            {
                if (infoFound)
                    vec.push_back(line);

                continue;
            }

            if (line.find(dir) != std::string::npos)
            {
                infoFound = true;
                vec.push_back(line);
            }
        }

        file.close();
    }

    return infoFound;
}

// -----------------------------------------------------------------------------------------------

bool myApp::rstr_ParseInfoVec(std::vector<std::string> &vec, std::map<std::string, std::string> &m1, std::map<std::string, std::string> &m2)
{
    // Gets the line 'C:\test\dir\file.jpg = > _tmp_001.jpg  -- Ok'
    // and performs: map["_tmp_001.jpg"] = "C:\test\dir\file.jpg"
    auto parse_line = [](std::map<std::string, std::string> &map, const std::string *str) -> bool
    {
        size_t pos = str->find(myGlobals::ren_to);

        if (pos != std::string::npos)
        {
            std::string val = str->substr(0u, pos);                     // 'C:\test\dir\file.jpg'
            std::string key = str->substr(pos + 4u, str->length());     // '_tmp_001.jpg  -- Ok'

            val = val.substr(val.find_last_of('\\') + 1u, val.length());
            key = key.substr(0u, key.find(myGlobals::ren_ok));

            map[key] = val;
            return true;
        }

        return false;
    };

    // -----------------------------------------------------------

    m1.clear();
    m2.clear();

    for (auto iter = vec.begin(); iter != vec.end(); ++iter)
    {
        std::string* str = &(*iter);

        const char* s = str->c_str() + str->length() - 5u;

        // One of the lines does not end with "-- Ok"
        if (strcmp(s, myGlobals::ren_ok + 2u) != 0)
            return false;

        if (str->find("=> _tmp_") != std::string::npos)
        {
            if (!parse_line(m1, str))
                return false;
        }
        else
        {
            if (!parse_line(m2, str))
                return false;
        }
    }

    return true;
}

// -----------------------------------------------------------------------------------------------

bool myApp::rstr_Rename(std::map<std::string, std::string> &m1, std::map<std::string, std::string> &m2, int &cnt)
{
    auto ren = [this](std::map<std::string, std::string> &map, const std::string &dir, int &cnt)
    {
        cnt = 0;

        for (auto iter = map.begin(); iter != map.end(); ++iter)
        {
            std::string oldName = dir + iter->first;
            std::string newName = dir + iter->second;

            std::cout << myGlobals::tab_of_four << oldName << " => " << newName << ":\n" << myGlobals::tab_of_four << myGlobals::tab_of_four << "Status: ";

            if (!fileExists(oldName))
            {
                std::cout << "Fail (src file does not exist)\n";
                cnt++;
                continue;
            }

            if (fileExists(newName))
            {
                std::cout << "Fatal Error (dest file already exists)\n";
                return false;
            }

            if (MoveFileA(oldName.c_str(), newName.c_str()) == 0)
            {
                std::cout << "Fatal Error (renaming operation unsuccessful)\n";
                return false;
            }

            std::cout << "Ok\n";
        }

        return true;
    };

    auto upd_info = [](const std::map<std::string, std::string>& map, std::string &info, int num, int cnt) -> void
    {
        if (cnt)
        {
            info += " Renaming iteration " + std::to_string(num) + ": "
                                           + std::to_string(cnt) + "/" + std::to_string(map.size()) + " src file(s) weren't found)\n";
        }
    };

    // ----------------------------------------------------

    std::string str;

    if (!ren(m1, _dir, cnt))
        return false;

    upd_info(m1, str, 1, cnt);

    if (!ren(m2, _dir, cnt))
        return false;

    upd_info(m2, str, 2, cnt);

    if (!str.empty())
    {
        std::cout << "\n" << str;
    }

    return true;
}

// -----------------------------------------------------------------------------------------------

// Moves up the directory path and returns the new path
std::string myApp::getUpperLevelDirectory(const std::string &path, size_t level) const
{
	size_t pos(path.length() - 1u), cnt(0u);

	while (cnt != level)
		if (path[--pos] == '\\')
			cnt++;

	return path.substr(0, pos + 1u);
}

// -----------------------------------------------------------------------------------------------

std::string myApp::getCurrentDirFromPath(const std::string &path) const
{
    size_t len = path.length();

    if (len == 3u)
        return path;

    size_t pos(len - 1u), cnt(path[pos] == '\\' ? 1u : 0u);

    while (path[pos - cnt] != '\\')
        pos--;

    return path.substr(pos, len - pos - cnt);
}

// -----------------------------------------------------------------------------------------------

void myApp::logError(const std::string &str)
{
    std::fstream file;

    file.open(_dir + "error.log", std::ios::app);

    if (file.is_open())
    {
        file.write(str.c_str(), str.length());
        file.write("\n", 1u);
        file.close();
    }
}

// -----------------------------------------------------------------------------------------------
