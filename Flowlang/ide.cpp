// ide.cpp
// FlowLang Win32 GUI IDE (no external libraries)

#include <windows.h>
#include <string>
#include <fstream>
#include <sstream>

#define ID_EDITOR   101
#define ID_OUTPUT   102
#define ID_RUN      201
#define ID_AI       202
#define ID_SAVE     203
#define ID_OPEN     204
#define ID_PKG      205

std::string currentFile = "main.flow";

std::string runCommand(const std::string& cmd) {
    std::string result;
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return "Error running command.";
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe)) {
        result += buffer;
    }
    _pclose(pipe);
    return result;
}

void loadFile(HWND hEdit) {
    std::ifstream in(currentFile);
    if (!in) return;
    std::stringstream ss;
    ss << in.rdbuf();
    std::string text = ss.str();
    SetWindowTextA(hEdit, text.c_str());
}

void saveFile(HWND hEdit) {
    int len = GetWindowTextLengthA(hEdit);
    char* buf = new char[len + 1];
    GetWindowTextA(hEdit, buf, len + 1);
    std::ofstream out(currentFile);
    out << buf;
    delete[] buf;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hEdit, hOutput;

    switch (msg) {
    case WM_CREATE:
        hEdit = CreateWindowExA(
            WS_EX_CLIENTEDGE, "EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
            10, 10, 760, 400,
            hwnd, (HMENU)ID_EDITOR, GetModuleHandle(NULL), NULL);

        hOutput = CreateWindowExA(
            WS_EX_CLIENTEDGE, "EDIT", "",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            10, 420, 760, 200,
            hwnd, (HMENU)ID_OUTPUT, GetModuleHandle(NULL), NULL);

        CreateWindowA("BUTTON", "Run", WS_CHILD | WS_VISIBLE,
            780, 10, 120, 40, hwnd, (HMENU)ID_RUN, NULL, NULL);

        CreateWindowA("BUTTON", "AI Generate", WS_CHILD | WS_VISIBLE,
            780, 60, 120, 40, hwnd, (HMENU)ID_AI, NULL, NULL);

        CreateWindowA("BUTTON", "Save", WS_CHILD | WS_VISIBLE,
            780, 110, 120, 40, hwnd, (HMENU)ID_SAVE, NULL, NULL);

        CreateWindowA("BUTTON", "Open", WS_CHILD | WS_VISIBLE,
            780, 160, 120, 40, hwnd, (HMENU)ID_OPEN, NULL, NULL);

        CreateWindowA("BUTTON", "Packages", WS_CHILD | WS_VISIBLE,
            780, 210, 120, 40, hwnd, (HMENU)ID_PKG, NULL, NULL);

        loadFile(hEdit);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_SAVE:
            saveFile(hEdit);
            break;

        case ID_OPEN:
            loadFile(hEdit);
            break;

        case ID_RUN: {
            saveFile(hEdit);
            std::string out = runCommand("flowlang " + currentFile);
            SetWindowTextA(hOutput, out.c_str());
            break;
        }

        case ID_AI: {
            char prompt[256];
            if (DialogBoxParamA(NULL, MAKEINTRESOURCEA(1), hwnd, NULL, NULL)) {}
            std::string out = runCommand("flowlang --ai \"write something\"");
            SetWindowTextA(hOutput, out.c_str());
            break;
        }

        case ID_PKG: {
            std::string out = runCommand("flowlang pkg list");
            SetWindowTextA(hOutput, out.c_str());
            break;
        }
        }
        break;

    case WM_SIZE:
        MoveWindow(hEdit, 10, 10, LOWORD(lParam) - 240, HIWORD(lParam) - 220, TRUE);
        MoveWindow(hOutput, 10, HIWORD(lParam) - 200, LOWORD(lParam) - 240, 190, TRUE);
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    WNDCLASSA wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = "FlowLangIDE";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowA(
        "FlowLangIDE", "FlowLang IDE",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 950, 680,
        NULL, NULL, hInst, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}
