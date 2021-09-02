#include "myApp.h"

// -----------------------------------------------------------------------------------------------

void myApp::selectOption()
{
	char ch(0);

	std::cout << std::endl;
	std::cout << " Select option:\n\n";
	std::cout << " 1. Rearrange directories depending on files size/dimensions (Before the resize)\n";
	std::cout << " 2. Analyze the files (After the resize)\n";
	std::cout << " 3. Restore original file names\n";
	std::cout << " 0. Exit\n";
	std::cout << " >> ";

	std::cin >> ch;

	switch (ch)
	{
		case '1':
			_mode = 1;
			break;

		case '2':
			_mode = 2;
			break;

		case '3':
			_mode = 3;
			break;
	}

	return;
}

// -----------------------------------------------------------------------------------------------

void myApp::process()
{
	selectOption();

	switch (_mode)
	{
		case 1: {

				// Read file structure and get files info
				std::cout << " Reading files..." << std::endl;
				readFiles(_dir);

				// Rename files (optional, will be asked about it)
				std::cout << " Renaming files..." << std::endl;
				renameFiles();

				// Move directories
				std::cout << " Rearranging directories..." << std::endl;
				sortDirs();

				std::cout << " Done: OK" << std::endl;

			}
			break;

		case 2: {

				std::cout << " Checking files:" << std::endl;
				checkOnRenamed();

			}
			break;

		case 3: {

				std::cout << " Not implemented yet =(" << std::endl;

			}
			break;

		default:
			std::cout << " Exiting..." << std::endl;
	}

	std::cout << " Press any key to contiunue : " << std::endl;

	getchar();

	return;
}

// -----------------------------------------------------------------------------------------------

// Get list of directories within the directory
void myApp::findDirs(std::string dir)
{
	WIN32_FIND_DATA ffd;
	TCHAR szDir[MAX_PATH];
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
				if (name != "." && name != "..")
				{
					if (fullName != _dirQuality && fullName != _dirResize)
					{
						dirInfo di;
						di.fullName = fullName;

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
// Return total size of all files found
void myApp::findFiles(std::string dir, std::map<std::string, myApp::fileInfo, myCmp> &map)
{
	const char* validExtensions[] = { ".jpg", ".jpeg" };

	WIN32_FIND_DATA ffd;
	TCHAR szDir[MAX_PATH];
	HANDLE hFind = INVALID_HANDLE_VALUE;

	bool error(false);
	size_t size(0);
	map.clear();
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

			if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
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
						size = 0;
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

					map.emplace(name, inf);
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
	dir = _dir + dir;

	DWORD dwAttrib = GetFileAttributesA(dir.c_str());

	return (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
};

// -----------------------------------------------------------------------------------------------

void myApp::mkDir(std::string dir)
{
	dir = _dir + dir;

	CreateDirectoryA(dir.c_str(), NULL);

	return;
}

// -----------------------------------------------------------------------------------------------

void myApp::mvDir(std::string shortName, std::string fullName, enum DIRS d)
{
	std::string dest = _dir;

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
	}

	dest += shortName;

	MoveFileExA(fullName.c_str(), dest.c_str(), MOVEFILE_WRITE_THROUGH);

	return;
}

// -----------------------------------------------------------------------------------------------

void myApp::createAuxDirs()
{
	if (!dirExists("[__do_resize]"))
		mkDir("[__do_resize]");

	if (!dirExists("[__do_lower_quality]"))
		mkDir("[__do_lower_quality]");

	if (!dirExists("[__files.r]"))
		mkDir("[__files.r]");

	_dirResize  = _dir + "[__do_resize]\\";
	_dirQuality = _dir + "[__do_lower_quality]\\";
	_dirResized = _dir + "[__files.r]\\";

	return;
}

// -----------------------------------------------------------------------------------------------

void myApp::renameFiles()
{
	std::string data;

	// ------------------------------------------------------------------------

	auto ren = [&data](std::string oldName, std::string newName, size_t cnt = 0u, bool ext = true)
	{
		size_t pos = oldName.find_last_of('\\') + 1;

		// add new name
		std::string newFullName = oldName.substr(0, pos) + newName;

		// add counter
		if (cnt)
			newFullName += std::to_string(cnt);

		// add extension
		if (ext)
		{
			pos = oldName.find_last_of('.');

			if(pos != std::string::npos)
				newFullName += oldName.substr(pos, oldName.length());
		}

		if (oldName != newFullName)
		{
//			std::cout << " ren '" << oldName << "' to '" << newFullName << "'" << std::endl;

			MoveFileA(oldName.c_str(), newFullName.c_str());

			data += oldName;
			data += " => ";
			data += newFullName.substr(newFullName.find_last_of('\\') + 1u, newFullName.length());
			data += "\n";
		}

		return;
	};

	auto getNumericName = [](size_t num) -> std::string
	{
		std::string res = std::to_string(num);

		while (res.length() != 3u)
			res.insert(0, "0");

		return res;
	};

	// ------------------------------------------------------------------------

	static char option(0);

	if (!option)
	{
		std::cout << " Do you want to rename files in numeric order? (y/n): ";
		std::cin >> option;
		std::cout << std::endl;
	}

	// For each dir:
	for (auto iter = _mapDirs.begin(); iter != _mapDirs.end(); ++iter)
	{
		size_t cnt_cover(0u), cnt_z_cover(0u), cnt(0u), totalSize(0u);

		auto mapFiles = &(iter->second.files);

		bool resizedFound = false;

		// Try to find covers first:
		for (auto it = mapFiles->begin(); it != mapFiles->end(); ++it)
		{
			totalSize += it->second.size;

			if (it->second.isResized)
				resizedFound = true;

			if (it->second.canRename)
			{
				// cover
				if (it->first.find("cover") != std::string::npos)
				{
					if (it->first.find("clean") != std::string::npos)
					{
						ren(it->second.fullName, "z_cover", cnt_z_cover);
						cnt_z_cover++;
					}
					else
					{
						ren(it->second.fullName, "_cover", cnt_cover);
						cnt_cover++;
					}

					it->second.canRename = false;
				}
			}
		}

		// No cover files were found
		// Try to find files that are significantly smaller than average file size in this dir:
		if (cnt_cover + cnt_z_cover == 0u)
		{
			float avg = (float)totalSize / mapFiles->size();

			for (auto it = mapFiles->begin(); it != mapFiles->end(); ++it)
			{
				if (it->second.canRename)
				{
					if (avg / it->second.size > 8)
					{
						ren(it->second.fullName, "_cover;" + it->first, 0u, false);
						it->second.canRename = false;
					}
				}
			}
		}

		if (!resizedFound)
		{
			// For the rest of the files:
			for (auto it = mapFiles->begin(); it != mapFiles->end(); ++it)
			{
				if (it->second.canRename && !it->second.isResized)
				{
					if (option == 'y' || option == 'Y')
					{
						ren(it->second.fullName, getNumericName(++cnt));
						it->second.canRename = false;
					}
				}
			}
		}
	}

	std::fstream file;

	file.open(_dir + "\\[info].txt", std::fstream::in | std::fstream::app);

	if (file.is_open())
	{
		file.write(data.c_str(), data.length());
		file.close();
	}

	return;
}

// -----------------------------------------------------------------------------------------------

// Read info about all the files into map
void myApp::readFiles(std::string dir)
{
	findDirs(dir);

	std::cout << " Found " << _mapDirs.size() << " directories:\n" << std::endl;

	// For each dir:
	for (auto iter = _mapDirs.begin(); iter != _mapDirs.end(); ++iter)
	{
		std::cout << " " << iter->second.fullName << std::endl;

		// Get all files in this directory
		findFiles(iter->second.fullName, iter->second.files);

		int x, y;

		// For each file:
		for (auto it = iter->second.files.begin(); it != iter->second.files.end(); ++it)
		{
			GetImageSize(it->second.fullName.c_str(), x, y);

			it->second.width  = x;
			it->second.height = y;

//			std::cout << "\t" << it->first << " [" << x << " x " << y << "]" << std::endl;
		}
	}

	std::cout << std::endl;

	return;
}

// -----------------------------------------------------------------------------------------------

void myApp::sortDirs()
{
	const size_t THRESHOLD_RESIZE  = 3000;
	const size_t THRESHOLD_QUALITY = 15;

	// For each dir:
	for (auto iter = _mapDirs.begin(); iter != _mapDirs.end(); ++iter)
	{
		auto mapFiles = &(iter->second.files);

		int maxX(0), maxY(0), x(0), y(0);
		size_t SizeTotal(0u), SizeQuality(0u), SizeResize(0u), cntResize(0u), cntQuality(0u), cntResized(0u);
		float avg = 0.0f;

		// For each file:
		for (auto it = mapFiles->begin(); it != mapFiles->end(); ++it)
		{
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

				if (minSize > THRESHOLD_RESIZE)
				{
					SizeResize += *currentSize;
					cntResize++;
				}
			}
		}

		std::cout << "\n    Total Files (" << mapFiles->size() << "):" << std::endl;

		if (mapFiles->size())
		{
			float totalSize = (float)SizeTotal / (1024 * 1024);
			float avgSize = totalSize / mapFiles->size();

			std::cout << "    Total File Size = " << totalSize << " Mb (" << SizeTotal << ") bytes" << std::endl;
			std::cout << "    Avrg. File Size = " << avgSize << " Mb (" << SizeTotal / mapFiles->size() << ") bytes" << std::endl;
		}

		std::cout << "\n    Files to Resize (" << cntResize << "):" << std::endl;

		if (cntResize)
		{
			float totalSize = (float)SizeResize / (1024 * 1024);
			float avgSize = totalSize / cntResize;

			std::cout << "    Total File Size = " << totalSize << " Mb (" << SizeResize << ") bytes" << std::endl;
			std::cout << "    Avrg. File Size = " << avgSize << " Mb (" << SizeResize / cntResize << ") bytes" << std::endl;
		}

		std::cout << "\n    Files Over 1 Mb (" << cntQuality << "):" << std::endl;

		if (cntQuality)
		{
			float totalSize = (float)SizeQuality / (1024 * 1024);
			avg = totalSize / cntQuality;

			std::cout << "    Total File Size = " << totalSize << " Mb (" << SizeQuality << ") bytes" << std::endl;
			std::cout << "    Avrg. File Size = " << avg << " Mb (" << SizeQuality / cntQuality << ") bytes" << std::endl;
		}

		std::cout << std::endl;

		do
		{
			if (cntResized)
			{
				mvDir(iter->first, iter->second.fullName, myApp::DIRS::RESIZED);
				break;
			}

			if (cntResize)
			{
				mvDir(iter->first, iter->second.fullName, myApp::DIRS::RESIZE);
				break;
			}

			if (avg * 10 > THRESHOLD_QUALITY)
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

		if (pos != std::string::npos)
			return name.substr(pos - 2u, 3u) == ".r.";

		return false;
	};

	auto isCover = [](std::string name) -> bool
	{
		return name.find("cover") != std::string::npos;
	};

	// ------------------------------------------------------------------------

	std::string data;
	std::vector<std::string *> vec;

	vec.emplace_back(&_dirQuality);
	vec.emplace_back(&_dirResize);

	// Read file structure and get files info
	std::cout << " Reading files..." << std::endl;

	// Upper directories:
	for (size_t i = 0; i < vec.size(); i++)
	{
		readFiles(*vec[i]);

		// Inner directories
		for (auto iter = _mapDirs.begin(); iter != _mapDirs.end(); ++iter)
		{
			auto mapFiles = &(iter->second.files);

			data = " Files NOT resized:\n";

			std::map<std::string, myApp::fileInfo *> m;
			size_t cnt_original(0u), cnt_resized(0u), size_original(0u), size_resized(0u);

			for (auto it = mapFiles->begin(); it != mapFiles->end(); ++it)
				m[it->first] = &(it->second);

			// Files
			for (auto it = m.begin(); it != m.end(); ++it)
			{
				std::cout << it->first << std::endl;

				// Leave covers as they are
				if (isCover(it->first))
					continue;

				if (!isRenamed(it->first))
				{
					cnt_original++;

					std::string str = it->first.substr(0, it->first.find_last_of('.')) + ".r.jpg";

					// This original file has a '.r.jpg' sibling
					if (m.count(str))
					{
						cnt_resized++;

						size_original += it->second->size;
						size_resized  += m[str]->size;
					}
					else
					{
						data += "  ";
						data += it->first;
						data += '\n';
					}
				}
			}

			data += "\n";
			data += " Files resized:\n";
			data += "  Original size   : " + std::to_string((float)size_original / 1024 / 1024);
			data += '\n';
			data += "  Resized size    : " + std::to_string((float)size_resized  / 1024 / 1024);
			data += '\n';
			data += "  Saved disk space: " + std::to_string(((float)size_original / 1024 / 1024) - ((float)size_resized / 1024 / 1024));
			data += '\n';

			if (size_original <= size_resized)
				data += "\n  WOW! Resized files are actually LARGER! 0_o\n";

			std::fstream file;

			file.open(iter->second.fullName + "\\[info].txt", std::fstream::in | std::fstream::app);

			if (file.is_open())
			{
				file.write(data.c_str(), data.length());
				file.close();
			}
		}
	}

	getchar();


#if 0

	// For each dir:
	for (auto iter = _mapDirs.begin(); iter != _mapDirs.end(); ++iter)
	{
		size_t cnt_cover(0u), cnt_z_cover(0u), cnt(0u), totalSize(0u);

		auto mapFiles = &(iter->second.files);

		bool resizedFound = false;

		// Try to find covers first:
		for (auto it = mapFiles->begin(); it != mapFiles->end(); ++it)
		{
			totalSize += it->second.size;

			if (it->second.isResized)
				resizedFound = true;

			if (it->second.canRename)
			{
				// cover
				if (it->first.find("cover") != std::string::npos)
				{
					if (it->first.find("clean") != std::string::npos)
					{
						ren(it->second.fullName, "z_cover", cnt_z_cover);
						cnt_z_cover++;
					}
					else
					{
						ren(it->second.fullName, "_cover", cnt_cover);
						cnt_cover++;
					}

					it->second.canRename = false;
				}
			}
		}

		// No cover files were found
		// Try to find files that are significantly smaller than average file size in this dir:
		if (cnt_cover + cnt_z_cover == 0u)
		{
			float avg = (float)totalSize / mapFiles->size();

			for (auto it = mapFiles->begin(); it != mapFiles->end(); ++it)
			{
				if (it->second.canRename)
				{
					if (avg / it->second.size > 8)
					{
						ren(it->second.fullName, "_cover;" + it->first, 0u, false);
						it->second.canRename = false;
					}
				}
			}
		}

		if (!resizedFound)
		{
			// For the rest of the files:
			for (auto it = mapFiles->begin(); it != mapFiles->end(); ++it)
			{
				if (it->second.canRename && !it->second.isResized)
				{
					if (option == 'y' || option == 'Y')
					{
						ren(it->second.fullName, getNumericName(++cnt));
						it->second.canRename = false;
					}
				}
			}
		}
	}

	std::fstream file;

	file.open(_dir + "\\[info].txt", std::fstream::in | std::fstream::app);

	if (file.is_open())
	{
		file.write(data.c_str(), data.length());
		file.close();
	}

#endif
	return;
}

// -----------------------------------------------------------------------------------------------
