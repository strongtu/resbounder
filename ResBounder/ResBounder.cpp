// ResBounder.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <windows.h>

// pngout.exe srcfile.ext outfile.png /y /d0 /s0 /mincodes0

using namespace std;

struct GFTHeader
{
    DWORD   tgf;
    WORD    ver;
    WORD    reserve;
    DWORD   reserve2;
    WORD    textureMode;
    WORD    blendMode;
    DWORD   offset;
};

wstring g_PngOut;

bool CreateDir(wstring dir)
{
    return CreateDirectory(dir.c_str(), NULL);
}

bool IsTypeFile(const wchar_t * fileName, const wchar_t * type)
{
    size_t lenFileName = wcslen(fileName);
    size_t lenType = wcslen(type);

    if (lenFileName <= lenType)
    {
        return false;
    }
   
    return !wcsicmp(fileName + lenFileName - lenType, type);
}

bool ExtractGFTInnerImage(const wchar_t * gftFile, const wchar_t * imgFile)
{
    FILE * pfGFT = _wfopen(gftFile, _T("rb"));
    if (!pfGFT) return false;

    GFTHeader head;
    if (!fread(&head, sizeof(head), 1, pfGFT) ||
        head.tgf != 0x00464754)
    {
        fclose(pfGFT);
        return false;
    }

    FILE * pfIMG = _wfopen(imgFile, _T("wb"));
    if (!pfIMG)
    {
        fclose(pfGFT);
        return false;
    }

    fseek(pfGFT, head.offset, SEEK_SET);
    
    BYTE buf[512];
    int n = 0;
    while(n = fread(buf, 1, 512, pfGFT))
    {
        fwrite(buf, 1, n, pfIMG);
    }

    fclose(pfGFT);
    fclose(pfIMG);

    return true;
}

bool PngOutFile(const wchar_t * src, const wchar_t * dst)
{
    wstring ws =_T("pngout.exe ");
    ws +=_T("\"");
    ws += src; 
    ws += _T("\" ");
    ws += _T("\"");
    ws += dst;
    ws += + _T("\" ");
    ws += _T("/knpTc /y /d0 /s0 /mincodes0");

    PROCESS_INFORMATION ProcessInfo;
    STARTUPINFO StartupInfo;
    ZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof StartupInfo;
    if(CreateProcess(g_PngOut.c_str(), (LPWSTR)ws.c_str(), NULL, NULL, FALSE, 0, NULL, NULL, &StartupInfo, &ProcessInfo))
    {
        WaitForSingleObject(ProcessInfo.hProcess,INFINITE);
        CloseHandle(ProcessInfo.hThread);
        CloseHandle(ProcessInfo.hProcess);

        return true;
    }
    else
    {
        return false;
    }
}

bool ReplaceGFTInnerImage(const wchar_t * gftFile, const wchar_t * imgFile)
{
	DWORD dwAttr = GetFileAttributes(gftFile);
	dwAttr = dwAttr & ~FILE_ATTRIBUTE_READONLY;
	SetFileAttributes(gftFile, dwAttr);

    HANDLE pfGFT = CreateFile(gftFile, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, 0);
    if (INVALID_HANDLE_VALUE == pfGFT) return false;

    GFTHeader head;
    DWORD dwRead = 0;
    ReadFile(pfGFT, &head, sizeof(head), &dwRead, NULL);
    if (!dwRead || head.tgf != 0x00464754)
    {
        CloseHandle(pfGFT);
        return false;
    }

    SetFilePointer(pfGFT, head.offset, NULL, FILE_BEGIN);
    SetEndOfFile(pfGFT);

    HANDLE pfIMG = CreateFile(imgFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    if (INVALID_HANDLE_VALUE == pfIMG)
    {
        CloseHandle(pfGFT);
        return false;
    }
    
    BYTE buf[512];
    int n = 0;

    ReadFile(pfIMG, buf, 512, &dwRead, NULL);
    while(dwRead)
    {
        WriteFile(pfGFT, buf, dwRead, &dwRead, NULL);

        ReadFile(pfIMG, buf, 512, &dwRead, NULL);
    }

    CloseHandle(pfGFT);
    CloseHandle(pfIMG);

    return true;
}

bool IsFileSmall(const wchar_t * src, const wchar_t * dst)
{
	HANDLE pSrc = CreateFile(src, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
	if (INVALID_HANDLE_VALUE == pSrc)
	{
		return false;
	}

	HANDLE pDst = CreateFile(dst, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
	if (INVALID_HANDLE_VALUE == pDst)
	{
		CloseHandle(pSrc);
		return false;
	}

	// notic : you should not make a image large then 2G
	DWORD szSrc = GetFileSize(pSrc, NULL);
	DWORD szDst = GetFileSize(pDst, NULL);

	CloseHandle(pSrc);
	CloseHandle(pDst);

	return szDst < szSrc;
}

void BoundDir(std::wstring src, std::wstring dst, std::wstring backup)
{
    WIN32_FIND_DATA fileData;

    wstring filePath = src + _T("\\*.*");

    HANDLE hFind = ::FindFirstFile(filePath.c_str(), &fileData);
    if (INVALID_HANDLE_VALUE == hFind) return;

    do
    {
        if (!wcscmp(fileData.cFileName, _T(".")) ||
            !wcscmp(fileData.cFileName, _T("..")) ||
			(fileData.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
        {
            continue;
        }

        wstring srcFilePath = src + _T("\\") + fileData.cFileName;
        wstring dstFilePath = dst + _T("\\") + fileData.cFileName;
        wstring backupFilePath;
        
        if (!backup.empty())
        {
            backupFilePath = backup + _T("\\") + fileData.cFileName;
        }

        if (fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {   // enter
            CreateDir(dstFilePath);
            if (!backupFilePath.empty())
            {
                CreateDir(backupFilePath);
            }
            BoundDir(srcFilePath, dstFilePath, backupFilePath);
        }
        else if (IsTypeFile(fileData.cFileName, _T(".png")) && !IsTypeFile(fileData.cFileName, _T(".9.png")) ||
                 IsTypeFile(fileData.cFileName, _T(".bmp")))
        {
			if (!wcscmp(srcFilePath.c_str(), dstFilePath.c_str()) && !backupFilePath.empty())//?????идик?,?ииб└?бдY
			{
				CopyFile(srcFilePath.c_str(), backupFilePath.c_str(), FALSE);
			}

            if (PngOutFile(srcFilePath.c_str(), dstFilePath.c_str()) &&
                IsTypeFile(fileData.cFileName, _T(".bmp")))
            {
                wstring pngOutFile = dstFilePath + _T(".png");
                MoveFile(pngOutFile.c_str(), dstFilePath.c_str());
            }

			if (wcscmp(srcFilePath.c_str(), dstFilePath.c_str()))//????2??идик?,иж?3yбфиож╠?????
			{
				if (IsFileSmall(srcFilePath.c_str(), dstFilePath.c_str()))
				{
					if (!backupFilePath.empty())
					{
						CopyFile(srcFilePath.c_str(), backupFilePath.c_str(), FALSE);
					}
				}
				else
				{
					DeleteFile(dstFilePath.c_str());
				}
			}
        }
        else if (IsTypeFile(fileData.cFileName, _T(".gft")))
        {
            wstring gftInnerFile = dst;
            gftInnerFile += _T("\\");
            gftInnerFile += fileData.cFileName;
            gftInnerFile += _T(".png");

            if (ExtractGFTInnerImage(srcFilePath.c_str(), gftInnerFile.c_str()))
            {
                if (PngOutFile(gftInnerFile.c_str(), gftInnerFile.c_str()))
                {
                    CopyFile(srcFilePath.c_str(), dstFilePath.c_str(), FALSE);
                    ReplaceGFTInnerImage(dstFilePath.c_str(), gftInnerFile.c_str());
                    DeleteFile(gftInnerFile.c_str());
				}
 
            }

			if (IsFileSmall(srcFilePath.c_str(), dstFilePath.c_str()))
			{
				if (!backupFilePath.empty())
				{
					CopyFile(srcFilePath.c_str(), backupFilePath.c_str(), FALSE);
				}
			}
			else
			{
				DeleteFile(dstFilePath.c_str());
			}
        }
    }while(FindNextFile(hFind, &fileData));
}

int _tmain(int argc, _TCHAR* argv[])
{
    TCHAR modpath[1024] = {0};
	if (!GetModuleFileName(NULL, modpath, sizeof(modpath) / sizeof(modpath[0])))
	{
		return 0;
	}

    TCHAR drive[_MAX_DRIVE] = {0};
	TCHAR dir[_MAX_DIR] = {0};
	TCHAR fname[_MAX_FNAME] = {0};
	TCHAR ext[_MAX_EXT] = {0};

	_tsplitpath(modpath, drive, dir, fname, ext);

    g_PngOut = drive;
    g_PngOut += dir;
    g_PngOut += _T("pngout.exe");

    if (argc < 3)
    {
        printf("ResBounder %%SrcPath%% %%DstPath%% [%%BackupPath%%] \r\n");
        printf("press enter to exit!\r\n");
        getchar();
        return 0;
    }
    wstring srcDir = argv[1];
    wstring dstDir = argv[2];
    wstring backupDir;
    
    if (argc > 3)
    {
        backupDir = argv[3];
    }

    BoundDir(srcDir, dstDir, backupDir);
	return 0;
}

