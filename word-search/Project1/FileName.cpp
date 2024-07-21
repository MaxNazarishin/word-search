#include <windows.h>
#include <tchar.h>
#include "resource.h"
#include <CommCtrl.h>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime> 
using namespace std;

wstring word;
HWND hProgress;
int countSecond;
int totalReplacements1;
bool wordFound1;
bool stop = true;

void ClearResultFile()
{
    ofstream resultFile("result.txt", ios::out | ios::trunc);
    if (resultFile.is_open())
    {
        resultFile.close();
        stop = false;
    }
    else
    {
        wstring message = L"Error clearing result file 'result.txt'!";
        MessageBox(NULL, message.c_str(), L"Error", MB_OK | MB_ICONERROR);
    }
}

void CheckForResults(HWND hWnd, bool wordFound, int totalReplacements, const wstring& word)
{
    if (wordFound)
    {
        wstring message = L"The word '" + word + L"' is found in at least one file!";
        MessageBox(hWnd, message.c_str(), L"Information", MB_OK | MB_ICONINFORMATION);
    }
    else
    {
        wstring message = L"The word '" + word + L"' is not found in any file!";
        MessageBox(hWnd, message.c_str(), L"Information", MB_OK | MB_ICONINFORMATION);
    }

    if (totalReplacements > 0) {
        wstringstream ss;
        ss << "Total replacements: " << totalReplacements;
        MessageBox(hWnd, ss.str().c_str(), L"Total Replacements", MB_OK | MB_ICONINFORMATION);
    }
}

wstring ReplaceWordWithStars(const wstring& line, const wstring& word, int& replacements)
{
    wstring result = line;
    size_t pos = 0;
    while ((pos = result.find(word, pos)) != wstring::npos)
    {
        result.replace(pos, word.length(), word.length(), L'*');
        pos += word.length();
        replacements++;
    }
    return result;
}

void OpenAndCheckFile(const wstring& fileName, const wstring& word, wofstream& resultFile, int& replacements, bool& wordFound)
{
    ifstream file(fileName, ios::binary | ios::ate);
    if (file.is_open())
    {
        streampos fileSize = file.tellg();
        file.seekg(0, ios::beg);

        string line;
        while (getline(file, line))
        {
            wstring wstrLine(line.begin(), line.end());
            if (wstrLine.find(word) != wstring::npos)
            {
                wstrLine = ReplaceWordWithStars(wstrLine, word, replacements);
                wordFound = true;

                resultFile << wstrLine << endl;
            }
        }

        if (replacements > 0) {
            resultFile << "-----------------------------" << endl;
            resultFile << "File name: " << fileName << endl;
            resultFile << "File size: " << fileSize << " bytes" << endl;
            resultFile << "Number of replacements: " << replacements << endl;
            resultFile << "-----------------------------" << endl;
        }

        file.close();
    }
}

void SearchFilesForWord(const wstring& directory, const wstring& word)
{
    bool wordFound = false;
    int totalReplacements = 0; 

    wofstream resultFile("result.txt", ios::out | ios::trunc);
    if (resultFile.is_open())
    {
        WIN32_FIND_DATA findFileData;
        HANDLE hFind = FindFirstFile((directory + L"\\*.txt").c_str(), &findFileData);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                wstring fileName = findFileData.cFileName;
                int replacements = 0;
                OpenAndCheckFile(fileName, word, resultFile, replacements, wordFound);
                totalReplacements += replacements;

                totalReplacements1 = totalReplacements;
                wordFound1 = wordFound;
            } while (FindNextFile(hFind, &findFileData) != 0);
            FindClose(hFind);
        }
        resultFile.close();
    }
    else
    {
        wstring message = L"Error opening result file 'result.txt'!";
        MessageBox(NULL, message.c_str(), L"Error", MB_OK | MB_ICONERROR);
    }
}

DWORD WINAPI SearchThreadProc(LPVOID lpParam)
{
    HWND hEdit1 = GetDlgItem((HWND)lpParam, IDC_EDIT1);
    if (hEdit1 != NULL)
    {
        int textLength = GetWindowTextLength(hEdit1);
        if (textLength > 0)
        {
            wchar_t* buffer = new wchar_t[textLength + 1];
            GetWindowText(hEdit1, buffer, textLength + 1);
            word = buffer;
            delete[] buffer;

            SearchFilesForWord(L".", word);
        }

        if (textLength > 0) {
            srand(time(0));
            countSecond = rand() % 3 + 3;
            SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, countSecond));
            SetTimer((HWND)lpParam, 1, 1000, NULL);
        }
    }

    return 0;
}

void StartSearchThread(HWND hWnd)
{
    HANDLE hThread = CreateThread(NULL, 0, SearchThreadProc, hWnd, 0, NULL);
    if (hThread == NULL)
    {
        MessageBox(NULL, L"Failed to create search thread!", L"Error", MB_OK | MB_ICONERROR);
    }
    else
    {
        CloseHandle(hThread);
    }
}

int CALLBACK DlgProc(HWND hWnd, UINT mes, WPARAM wp, LPARAM lp)
{
    switch (mes)
    {
    case WM_INITDIALOG:
    {
        srand(time(0));
        hProgress = GetDlgItem(hWnd, IDC_PROGRESS1);
        SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 2));
        SendMessage(hProgress, PBM_SETSTEP, 1, 0);
        SendMessage(hProgress, PBM_SETPOS, 0, 0);
        SendMessage(hProgress, PBM_SETBKCOLOR, 0, LPARAM(RGB(0, 0, 255)));
        SendMessage(hProgress, PBM_SETBARCOLOR, 0, LPARAM(RGB(255, 255, 0)));
    }
    break;
    case WM_COMMAND:
    {
        switch (LOWORD(wp))
        {
        case IDC_BUTTON3:
        {
            StartSearchThread(hWnd);
        }
        break;
        case IDC_BUTTON5:
        {
            ClearResultFile();
            KillTimer(hWnd, 1);
            SendMessage(hProgress, PBM_SETPOS, 0, 0);
            stop = true;
        }
        break;
        }
        break;
    }
    case WM_TIMER:
    {
        int nPos = SendMessage(hProgress, PBM_GETPOS, 0, 0);
        if (nPos < countSecond)
        {
            SendMessage(hProgress, PBM_STEPIT, 0, 0);
        }
        else
        {
            KillTimer(hWnd, 1);
            SendMessage(hProgress, PBM_SETBKCOLOR, 0, LPARAM(RGB(0, 0, 255)));
            SendMessage(hProgress, PBM_SETPOS, 0, 0);

            if (stop && word.size() > 0) {
                CheckForResults(hWnd, wordFound1, totalReplacements1, word);
            }
        }
        break;
    }
    case WM_CLOSE:
    {
        EndDialog(hWnd, 0);
        return TRUE;
    }
    }
    return FALSE;
}

int WINAPI _tWinMain(HINSTANCE hInst, HINSTANCE hPrev, LPTSTR lpszCmdLine, int nCmdShow)
{
    return DialogBox(hInst, MAKEINTRESOURCE(IDD_DIALOG1), NULL, (DLGPROC)DlgProc);
}