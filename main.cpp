#include <windows.h>
#include <urlmon.h>

#include <fstream>
#include <string>

#pragma comment(lib, "urlmon.lib")

// ----------------------------------------------------
// 파일에서 특정 JSON 문자열 값을 간단히 읽는다.
// 테스트용 최소 구현.
// ----------------------------------------------------

std::string ReadJsonValue(
    const std::string& filePath,
    const std::string& key
)
{
    std::ifstream file(filePath);

    if (!file.is_open())
        return "";

    std::string content(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );

    std::string target = "\"" + key + "\"";

    size_t keyPos = content.find(target);

    if (keyPos == std::string::npos)
        return "";

    size_t colonPos = content.find(':', keyPos);

    if (colonPos == std::string::npos)
        return "";

    size_t firstQuote = content.find('"', colonPos);

    if (firstQuote == std::string::npos)
        return "";

    size_t secondQuote =
        content.find('"', firstQuote + 1);

    if (secondQuote == std::string::npos)
        return "";

    return content.substr(
        firstQuote + 1,
        secondQuote - firstQuote - 1
    );
}


// ----------------------------------------------------
// 현재 exe가 있는 폴더
// ----------------------------------------------------

std::string GetExeDirectory()
{
    char path[MAX_PATH] = {};

    GetModuleFileNameA(
        nullptr,
        path,
        MAX_PATH
    );

    std::string result = path;

    size_t pos = result.find_last_of("\\/");

    if (pos != std::string::npos)
        result.resize(pos + 1);

    return result;
}


// ----------------------------------------------------
// 업데이트 확인
// ----------------------------------------------------

bool CheckAndRunUpdate()
{
    std::string folder = GetExeDirectory();

    std::string localJson =
        folder + "version.json";

    std::string downloadedJson =
        folder + "latest.json";

    std::string setupPath =
        folder + "Setup_update.exe";


    // GitHub에 있는 최신 버전 정보
    const char* latestJsonUrl =
        "https://raw.githubusercontent.com/"
        "red-star939/updatetest/"
        "update_test-v0.0.2/"
        "latest.json";


    // ------------------------------------------
    // latest.json 다운로드
    // ------------------------------------------

    HRESULT hr = URLDownloadToFileA(
        nullptr,
        latestJsonUrl,
        downloadedJson.c_str(),
        0,
        nullptr
    );

    if (FAILED(hr))
        return false;


    // ------------------------------------------
    // 버전 읽기
    // ------------------------------------------

    std::string currentVersion =
        ReadJsonValue(
            localJson,
            "version"
        );

    std::string latestVersion =
        ReadJsonValue(
            downloadedJson,
            "version"
        );

    if (
        currentVersion.empty() ||
        latestVersion.empty()
    )
    {
        return false;
    }


    // 테스트 단계:
    // 현재는 0.0.1 -> 0.0.2이므로
    // 문자열이 다르면 업데이트한다.
    //
    // 나중에는 정식 버전 비교 함수를 만든다.

    if (currentVersion == latestVersion)
        return false;


    // ------------------------------------------
    // Setup URL 읽기
    // ------------------------------------------

    std::string setupUrl =
        ReadJsonValue(
            downloadedJson,
            "setup_url"
        );

    if (setupUrl.empty())
        return false;


    // ------------------------------------------
    // Setup.exe 다운로드
    // ------------------------------------------

    hr = URLDownloadToFileA(
        nullptr,
        setupUrl.c_str(),
        setupPath.c_str(),
        0,
        nullptr
    );

    if (FAILED(hr))
    {
        MessageBoxA(
            nullptr,
            "Setup download failed.",
            "Update",
            MB_ICONERROR
        );

        return false;
    }


    // ------------------------------------------
    // NSIS 설치 프로그램 실행
    // ------------------------------------------

    HINSTANCE result =
        ShellExecuteA(
            nullptr,
            "open",
            setupPath.c_str(),
            nullptr,
            folder.c_str(),
            SW_SHOWNORMAL
        );

    if ((INT_PTR)result <= 32)
    {
        MessageBoxA(
            nullptr,
            "Setup execution failed.",
            "Update",
            MB_ICONERROR
        );

        return false;
    }


    // Updater가 실행됐으므로
    // 현재 v0.0.1 종료
    return true;
}


// ----------------------------------------------------
// Window
// ----------------------------------------------------

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

    return DefWindowProcA(
        hwnd,
        uMsg,
        wParam,
        lParam
    );
}


// ----------------------------------------------------
// Main
// ----------------------------------------------------

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int nCmdShow
)
{
    // ------------------------------------------
    // 프로그램 시작 즉시 업데이트 확인
    // ------------------------------------------

    if (CheckAndRunUpdate())
    {
        return 0;
    }


    // ------------------------------------------
    // 업데이트가 없으면 기존 프로그램 실행
    // ------------------------------------------

    std::string folder =
        GetExeDirectory();

    std::string version =
        ReadJsonValue(
            folder + "version.json",
            "version"
        );

    if (version.empty())
        version = "unknown";


    std::string title =
        "Update Test v" + version;


    const char CLASS_NAME[] =
        "UpdateTestWindow";


    WNDCLASSA wc = {};

    wc.lpfnWndProc =
        WindowProc;

    wc.hInstance =
        hInstance;

    wc.lpszClassName =
        CLASS_NAME;

    wc.hCursor =
        LoadCursor(
            nullptr,
            IDC_ARROW
        );


    if (!RegisterClassA(&wc))
        return 0;


    HWND hwnd =
        CreateWindowExA(
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


    ShowWindow(
        hwnd,
        nCmdShow
    );

    UpdateWindow(hwnd);


    MSG msg = {};


    while (
        GetMessageA(
            &msg,
            nullptr,
            0,
            0
        ) > 0
    )
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }


    return static_cast<int>(
        msg.wParam
    );
}