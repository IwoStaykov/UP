#include "CameraProject.h"

#pragma comment(lib, "strmiids.lib") // DirectShow library

// Forward declaration of the Window Procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// DirectShow interfaces
IGraphBuilder* pGraph = NULL;
IMediaControl* pControl = NULL;
IVideoWindow* pWindow = NULL;
ICaptureGraphBuilder2* pBuilder = NULL;

void InitDirectShow(HWND hwnd) {
    // Initialize COM
    CoInitialize(NULL);

    // Create the filter graph
    CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&pGraph);

    // Create the capture graph builder
    CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC_SERVER, IID_ICaptureGraphBuilder2, (void**)&pBuilder);

    // Set the filter graph for the capture graph builder
    pBuilder->SetFiltergraph(pGraph);

    // Enumerate the video capture devices (USB cameras)
    ICreateDevEnum* pDevEnum = NULL;
    IEnumMoniker* pEnum = NULL;
    IMoniker* pMoniker = NULL;

    // Create the system device enumerator
    CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, IID_ICreateDevEnum, (void**)&pDevEnum);

    // Create an enumerator for video capture devices
    pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnum, 0);

    if (pEnum == NULL) {
        MessageBox(hwnd, L"No video capture device found", L"Error", MB_OK);
        return;
    }

    // Use the first video device
    pEnum->Next(1, &pMoniker, NULL);

    IBaseFilter* pCap = NULL;
    pMoniker->BindToObject(NULL, NULL, IID_IBaseFilter, (void**)&pCap);

    // Add the video capture filter to the filter graph
    pGraph->AddFilter(pCap, L"Capture Filter");

    // Render the video capture stream to the application window
    pBuilder->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video, pCap, NULL, NULL);

    // Query for the IMediaControl interface to control the graph
    pGraph->QueryInterface(IID_IMediaControl, (void**)&pControl);

    // Query for the IVideoWindow interface to control the video display
    pGraph->QueryInterface(IID_IVideoWindow, (void**)&pWindow);

    // Set the window to display the video
    pWindow->put_Owner((OAHWND)hwnd); // Set the owner of the video window to the main window
    pWindow->put_WindowStyle(WS_CHILD | WS_CLIPSIBLINGS); // Set the window style

    // Position the video window
    RECT rc;
    GetClientRect(hwnd, &rc);
    pWindow->SetWindowPosition(0, 0, rc.right, rc.bottom);

    // Run the graph to start video capture
    pControl->Run();

    // Release resources
    pDevEnum->Release();
    pEnum->Release();
    pMoniker->Release();
    pCap->Release();
}

void CleanupDirectShow() {
    if (pControl) {
        pControl->Stop();
        pControl->Release();
    }
    if (pWindow) {
        pWindow->put_Visible(OAFALSE); // Hide the video window
        pWindow->put_Owner(NULL); // Detach the video window from the main window
        pWindow->Release();
    }
    if (pGraph) {
        pGraph->Release();
    }
    if (pBuilder) {
        pBuilder->Release();
    }
    CoUninitialize();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR pCmdLine, int nCmdShow) {
    const wchar_t CLASS_NAME[] = L"Sample Window Class";

    WNDCLASS wc = { };
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles
        CLASS_NAME,                     // Window class
        L"USB Camera Display",          // Window title
        WS_OVERLAPPEDWINDOW,            // Window style
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        NULL,                           // Parent window    
        NULL,                           // Menu
        hInstance,                      // Instance handle
        NULL                            // Additional application data
    );

    if (hwnd == NULL) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);

    // Initialize DirectShow for capturing from a USB camera
    InitDirectShow(hwnd);

    // Main message loop
    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup DirectShow resources
    CleanupDirectShow();

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE: {
        // Resize the video window when the main window is resized
        if (pWindow) {
            RECT rc;
            GetClientRect(hwnd, &rc);
            pWindow->SetWindowPosition(0, 0, rc.right, rc.bottom);
        }
        return 0;
    }

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}
