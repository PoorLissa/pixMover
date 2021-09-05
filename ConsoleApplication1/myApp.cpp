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

				createAuxDirs(true);

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

				createAuxDirs(false);
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
// Return total size of all files found
void myApp::findFiles(std::string dir, dirInfo &di)
{
	const char* validExtensions[] = { ".jpg", ".jpeg" };

	WIN32_FIND_DATA ffd;
	TCHAR szDir[MAX_PATH];
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

		case DIRS::SKIPPED:
			dest = _dirSkipped;
			break;
	}

	dest += shortName;

	MoveFileExA(fullName.c_str(), dest.c_str(), MOVEFILE_WRITE_THROUGH);

	return;
}

// -----------------------------------------------------------------------------------------------

void myApp::createAuxDirs(bool doCreate)
{
	if (doCreate)
	{
		if (!dirExists("[__do_resize]"))
			mkDir("[__do_resize]");

		if (!dirExists("[__do_lower_quality]"))
			mkDir("[__do_lower_quality]");

		if (!dirExists("[__files.r]"))
			mkDir("[__files.r]");

		if (!dirExists("[__skipped]"))
			mkDir("[__skipped]");
	}

	_dirResize  = _dir + "[__do_resize]\\";
	_dirQuality = _dir + "[__do_lower_quality]\\";
	_dirResized = _dir + "[__files.r]\\";
	_dirSkipped = _dir + "[__skipped]\\";

	return;
}

// -----------------------------------------------------------------------------------------------

void myApp::renameFiles()
{
	std::string data;

	// ------------------------------------------------------------------------

	auto ren = [&data](std::string oldName, std::string newName, std::string &newFullName, size_t cnt = 0u, bool ext = true) -> bool
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

			if(pos != std::string::npos)
				newFullName += oldName.substr(pos, oldName.length());
		}

		if (oldName != newFullName)
		{
//			std::cout << " ren '" << oldName << "' to '" << newFullName << "'" << std::endl;

			BOOL b = MoveFileA(oldName.c_str(), newFullName.c_str());

			data += oldName;
			data += " => ";
			data += newFullName.substr(newFullName.find_last_of('\\') + 1u, newFullName.length());
			data += b ? "  -- Ok" : "  -- Fail";
			data += "\n";

			return b != 0;
		}

		return false;
	};

	auto getNumericName = [](size_t num) -> std::string
	{
		std::string res = std::to_string(num);

		while (res.length() != 3u)
			res.insert(0, "0");

		return res;
	};

	// ------------------------------------------------------------------------

	static char opt_RenameFiles(0);
	static char opt_RenameCovers(0);
	std::string newFullName;

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

	// For each dir:
	for (auto iter = _mapDirs.begin(); iter != _mapDirs.end(); ++iter)
	{
		bool resizedFound = false;

		// Skip any folder that has subfolders
		if (iter->second.mode == 1)
			continue;

		size_t cnt_cover(0u), cnt_z_cover(0u), cnt(0u), totalSize(0u);

		auto mapFiles = &(iter->second.files);

		// Try to find out if any of the files were resized already:
		for (auto it = mapFiles->begin(); it != mapFiles->end(); ++it)
		{
			totalSize += it->second.size;

			if (it->second.isResized)
				resizedFound = true;
		}

		// No need to rename anything
		if (resizedFound)
		{
			continue;
		}

		// Try to find covers first:
		for (auto it = mapFiles->begin(); it != mapFiles->end(); ++it)
		{
			if (it->second.canRename)
			{
				// Cover found
				if (it->first.find("cover") != std::string::npos)
				{
					// Mark it, so it would not be renamed later
					it->second.canRename = false;

					if (opt_RenameCovers == 'y' || opt_RenameCovers == 'Y')
					{
						if (it->first.find("clean") != std::string::npos)
						{
							ren(it->second.fullName, "z_cover", newFullName, cnt_z_cover);
							cnt_z_cover++;
						}
						else
						{
							ren(it->second.fullName, "_cover", newFullName, cnt_cover);
							cnt_cover++;
						}
					}
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
						if (opt_RenameCovers == 'y' || opt_RenameCovers == 'Y')
						{
							ren(it->second.fullName, "_cover;" + it->first, newFullName, 0u, false);
							it->second.canRename = false;
						}
					}
				}
			}
		}

		// Rename the rest of the files:
		std::vector<std::string> vec;
			
		// Pass 1, to temporary name:
		for (auto it = mapFiles->begin(); it != mapFiles->end(); ++it)
		{
			if (it->second.canRename && !it->second.isResized)
			{
				if (opt_RenameFiles == 'y' || opt_RenameFiles == 'Y')
				{
					std::string tmpName("_tmp_" + getNumericName(++cnt));

					ren(it->second.fullName, tmpName, newFullName);
					it->second.canRename = false;

					vec.push_back(newFullName);
				}
			}
		}

		cnt = 0u;

		// Pass 2, final:
		for (auto it = vec.begin(); it != vec.end(); ++it)
		{
			ren(*it, getNumericName(++cnt), newFullName);
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
	int x(0), y(0);
	size_t i(0u);

	findDirs(dir);

	std::cout << " Found " << _mapDirs.size() << " directories:\n" << std::endl;

	// For each dir:
	for (auto iter = _mapDirs.begin(); iter != _mapDirs.end(); ++iter)
	{
		std::cout << "   " << ++i << ": " << iter->second.fullName << std::endl;

		// Get all files in this directory
		findFiles(iter->second.fullName, iter->second);

		switch (iter->second.mode)
		{
			case 0: {

					// For each file:
					for (auto it = iter->second.files.begin(); it != iter->second.files.end(); ++it)
					{
						GetImageSize(it->second.fullName.c_str(), x, y);

						it->second.width = x;
						it->second.height = y;

//						std::cout << "\t" << it->first << " [" << x << " x " << y << "]" << std::endl;
					}
				}
				break;

			case 1: {

					std::cout << "    --> Has subfolders. Will be skipped" << std::endl;

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
	const size_t THRESHOLD_RESIZE  = 3000;
	const size_t THRESHOLD_QUALITY = 15;

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
		}
		else
		{
			if (iter->second.mode == 1)
				doSkip = true;
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

		if (doSkip)
		{
			std::cout << "    Folder will be skipped" << std::endl;
		}

		std::cout << std::endl;

		do
		{
			if (doSkip)
			{
				mvDir(iter->first, iter->second.fullName, myApp::DIRS::SKIPPED);
				break;
			}

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
	std::cout << " Reading files..." << std::endl;

	// Upper level directories:
	for (size_t i = 0; i < vec.size(); i++)
	{
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
				std::cout << it->first << std::endl;

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
				data += "  ";
				data += it->first;
				data += '\n';
			}

			data += "\n";
			data += " - Files resized (" + std::to_string(cnt_resized) + ") -\n";
			data += "  Original size    : " + std::to_string((float)size_original / 1024 / 1024);
			data += '\n';
			data += "  Resized size     : " + std::to_string((float)size_resized  / 1024 / 1024);
			data += '\n';
			data += "  Saved disk space : " + std::to_string(((float)size_original / 1024 / 1024) - ((float)size_resized / 1024 / 1024));
			data += '\n';

			if (size_original <= size_resized)
				data += "\n  WOW! Resized files are actually LARGER! 0_o\n";

			// Move files into another directory
			if (map_not_resized.size() > 4u)
			{
				std::string dir = iter->second.fullName + "[-not-resized-]";
				CreateDirectoryA(dir.c_str(), NULL);

				for (auto it = map_not_resized.begin(); it != map_not_resized.end(); ++it)
				{
					std::string src = it->second;
					std::string dst = it->second;

					size_t pos = dst.find_last_of('\\') + 1;

					dst.insert(pos, "[-not-resized-]\\");

					MoveFileExA(src.c_str(), dst.c_str(), MOVEFILE_WRITE_THROUGH);
				}
			}

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

	return;
}

// -----------------------------------------------------------------------------------------------
