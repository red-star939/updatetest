#include <windows.h>
#include <fstream>
#include <string>

std::string ReadVersion()
{
    std::ifstream file("version.json");

    if (!file.is_open())
        return "unknown";

    std::string line;

    while (std::getline(file, line))
    {
        size_t keyPos = line.find("\"version\"");

        if (keyPos == std::string::npos)
            continue;

        size_t colonPos = line.find(':', keyPos);

        if (colonPos == std::string::npos)
            continue;

        size_t firstQuote = line.find('"', colonPos);

        if (firstQuote == std::string::npos)
            continue;

        size_t secondQuote = line.find('"', firstQuote + 1);

        if (secondQuote == std::string::npos)
            continue;

        return line.substr(
            firstQuote + 1,
            secondQuote - firstQuote - 1
        );
    }

    return "unknown";
}

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (uMsg)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int nCmdShow
)
{
    std::string version = ReadVersion();

    std::string title =
        "Update Test v" + version;

    const char CLASS_NAME[] =
        "UpdateTestWindow";

    WNDCLASSA wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassA(&wc))
        return 0;

    HWND hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        500,
        300,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hwnd)
        return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};

    while (GetMessageA(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return static_cast<int>(msg.wParam);
}