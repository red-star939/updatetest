#include <windows.h>
#include <urlmon.h>
#include <shellapi.h>

#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#pragma comment(lib, "urlmon.lib")
#pragma comment(lib, "shell32.lib")


// ============================================================
// JSON에서 문자열 값 하나를 읽는 간단한 함수
//
// 예:
// {
//     "version": "0.0.2"
// }
//
// ReadJsonValue(path, "version")
// -> "0.0.2"
//
// 현재 테스트 프로젝트용 최소 구현이다.
// ============================================================

std::string ReadJsonValue(
    const std::string& filePath,
    const std::string& key
)
{
    std::ifstream file(filePath, std::ios::binary);

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


// ============================================================
// 현재 실행 중인 test.exe가 있는 폴더를 가져온다.
//
// 예:
// C:\Users\USER\AppData\Local\UpdateTest\
// ============================================================

std::string GetExeDirectory()
{
    char path[MAX_PATH] = {};

    DWORD length = GetModuleFileNameA(
        nullptr,
        path,
        MAX_PATH
    );

    if (length == 0)
        return "";

    std::string result(path);

    size_t pos =
        result.find_last_of("\\/");

    if (pos == std::string::npos)
        return "";

    return result.substr(0, pos + 1);
}


// ============================================================
// 버전 문자열을 숫자 배열로 변환
//
// "0.0.3"
// -> { 0, 0, 3 }
//
// "1.12.4"
// -> { 1, 12, 4 }
// ============================================================

std::vector<int> ParseVersion(
    const std::string& version
)
{
    std::vector<int> result;

    std::stringstream stream(version);
    std::string part;

    while (std::getline(stream, part, '.'))
    {
        try
        {
            result.push_back(
                std::stoi(part)
            );
        }
        catch (...)
        {
            return {};
        }
    }

    return result;
}


// ============================================================
// latestVersion이 currentVersion보다 새로운지 비교
//
// 0.0.2 < 0.0.3 -> true
// 0.0.3 < 0.0.3 -> false
// 0.0.4 < 0.0.3 -> false
//
// 단순 문자열 비교를 하지 않는다.
// ============================================================

bool IsNewerVersion(
    const std::string& currentVersion,
    const std::string& latestVersion
)
{
    std::vector<int> current =
        ParseVersion(currentVersion);

    std::vector<int> latest =
        ParseVersion(latestVersion);

    if (current.empty() || latest.empty())
        return false;

    size_t count =
        (current.size() > latest.size())
        ? current.size()
        : latest.size();

    current.resize(count, 0);
    latest.resize(count, 0);

    for (size_t i = 0; i < count; ++i)
    {
        if (latest[i] > current[i])
            return true;

        if (latest[i] < current[i])
            return false;
    }

    return false;
}


// ============================================================
// 업데이트 확인 및 실행
// ============================================================

bool CheckAndRunUpdate()
{
    std::string folder =
        GetExeDirectory();

    if (folder.empty())
        return false;


    // --------------------------------------------------------
    // 로컬 파일
    // --------------------------------------------------------

    std::string localJson =
        folder + "version.json";

    std::string latestJson =
        folder + "latest.json";

    std::string setupPath =
        folder + "Setup_update.exe";


    // --------------------------------------------------------
    // 모든 버전이 공통으로 확인하는 URL
    //
    // v0.0.1
    // v0.0.2
    // v0.0.3
    // ...
    //
    // 모두 이 주소만 확인한다.
    // --------------------------------------------------------

    const char* latestJsonUrl =
        "https://raw.githubusercontent.com/"
        "red-star939/updatetest/"
        "main/"
        "latest.json";


    // --------------------------------------------------------
    // 이전에 받은 latest.json 삭제
    //
    // 오래된 파일을 잘못 읽는 상황을 줄인다.
    // --------------------------------------------------------

    DeleteFileA(
        latestJson.c_str()
    );


    // --------------------------------------------------------
    // GitHub에서 latest.json 다운로드
    // --------------------------------------------------------

    HRESULT hr =
        URLDownloadToFileA(
            nullptr,
            latestJsonUrl,
            latestJson.c_str(),
            0,
            nullptr
        );


    // 인터넷 연결 실패 등이 발생하면
    // 업데이트를 건너뛰고 프로그램을 정상 실행한다.

    if (FAILED(hr))
    {
        return false;
    }


    // --------------------------------------------------------
    // 현재 버전 읽기
    // --------------------------------------------------------

    std::string currentVersion =
        ReadJsonValue(
            localJson,
            "version"
        );


    // --------------------------------------------------------
    // GitHub의 최신 버전 읽기
    // --------------------------------------------------------

    std::string latestVersion =
        ReadJsonValue(
            latestJson,
            "version"
        );


    if (
        currentVersion.empty() ||
        latestVersion.empty()
    )
    {
        return false;
    }


    // --------------------------------------------------------
    // 버전 비교
    //
    // 예:
    //
    // 현재  0.0.2
    // 최신  0.0.3
    //
    // -> 업데이트 진행
    //
    // 현재  0.0.3
    // 최신  0.0.3
    //
    // -> 업데이트 안 함
    // --------------------------------------------------------

    if (!IsNewerVersion(
        currentVersion,
        latestVersion
    ))
    {
        return false;
    }


    // --------------------------------------------------------
    // latest.json에서 Setup.exe URL 읽기
    // --------------------------------------------------------

    std::string setupUrl =
        ReadJsonValue(
            latestJson,
            "setup_url"
        );

    if (setupUrl.empty())
    {
        return false;
    }


    // --------------------------------------------------------
    // 이전 업데이트 설치 파일 제거
    // --------------------------------------------------------

    DeleteFileA(
        setupPath.c_str()
    );


    // --------------------------------------------------------
    // GitHub Release의 Setup.exe 다운로드
    // --------------------------------------------------------

    hr =
        URLDownloadToFileA(
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
            "Update installer download failed.",
            "Update Test",
            MB_OK | MB_ICONERROR
        );

        return false;
    }


    // --------------------------------------------------------
    // 다운로드한 Setup.exe 실행
    // --------------------------------------------------------

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
            "Update installer could not be started.",
            "Update Test",
            MB_OK | MB_ICONERROR
        );

        return false;
    }


    // --------------------------------------------------------
    // Setup.exe가 실행됐으므로 현재 v0.0.2 종료
    //
    // 그래야 NSIS가 기존 test.exe를 안전하게 교체할 수 있다.
    // --------------------------------------------------------

    return true;
}


// ============================================================
// Window Procedure
// ============================================================

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (uMsg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;

        HDC hdc =
            BeginPaint(
                hwnd,
                &ps
            );

        const char* text =
            "Hello ZERO";

        TextOutA(
            hdc,
            200,
            120,
            text,
            lstrlenA(text)
        );

        EndPaint(
            hwnd,
            &ps
        );

        return 0;
    }


    case WM_CLOSE:
    {
        DestroyWindow(hwnd);
        return 0;
    }


    case WM_DESTROY:
    {
        PostQuitMessage(0);
        return 0;
    }
    }


    return DefWindowProcA(
        hwnd,
        uMsg,
        wParam,
        lParam
    );
}


// ============================================================
// Main
// ============================================================

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int nCmdShow
)
{
    // --------------------------------------------------------
    // 프로그램 창을 띄우기 전에 업데이트 확인
    // --------------------------------------------------------

    if (CheckAndRunUpdate())
    {
        // Setup_update.exe가 실행됐다.
        // 현재 버전은 즉시 종료.
        return 0;
    }


    // --------------------------------------------------------
    // 업데이트가 없으면 현재 프로그램 실행
    // --------------------------------------------------------

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
    {
        return 0;
    }


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
    {
        return 0;
    }


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