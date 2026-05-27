#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <glad/glad.h>
#include <string>
#include <functional>
#include <cstdlib>
#include <cstdio>
#include <cmath>

// ── Minimal Win32 + WGL window for OpenGL 3.3 Core + MSAA ──

struct Window {
    HWND     hwnd   = nullptr;
    HDC      hdc    = nullptr;
    HGLRC    hglrc  = nullptr;
    int      width  = 1280;
    int      height = 720;
    int      msaaSamples = 0;
    bool     shouldClose = false;
    bool     keys[256] = {};
    bool     mouseDown[3] = {};
    double   mouseX = 0, mouseY = 0;
    double   lastMouseX = 0, lastMouseY = 0;
    double   scrollDelta = 0;
    bool     mouseDragging = false;
    bool     resized = false;

    // Callbacks
    std::function<void(int, int)> onResize;
    std::function<void(float, float)> onMouseDrag;
    std::function<void(float)> onScroll;
    std::function<void(int, int)> onMouseButton;

    // ── Dummy window helper for WGL extension loading ──
    static HWND createDummyWindow() {
        HINSTANCE inst = GetModuleHandle(nullptr);
        WNDCLASS wc = {};
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = inst;
        wc.lpszClassName = "OGLDummy";
        RegisterClass(&wc);
        HWND h = CreateWindow("OGLDummy", "", WS_POPUP,
            0, 0, 1, 1, nullptr, nullptr, inst, nullptr);
        return h;
    }

    bool create(const char* title, int w, int h, bool msaa = true) {
        width = w; height = h;
        HINSTANCE inst = GetModuleHandle(nullptr);

        // 1. Create dummy window to get WGL extensions
        HWND dummyHWND = createDummyWindow();
        HDC  dummyHDC  = GetDC(dummyHWND);
        PIXELFORMATDESCRIPTOR dummyPFD = {};
        dummyPFD.nSize = sizeof(dummyPFD);
        dummyPFD.nVersion = 1;
        dummyPFD.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        dummyPFD.iPixelType = PFD_TYPE_RGBA;
        dummyPFD.cColorBits = 24;
        int dummyPF = ChoosePixelFormat(dummyHDC, &dummyPFD);
        SetPixelFormat(dummyHDC, dummyPF, &dummyPFD);
        HGLRC dummyCtx = wglCreateContext(dummyHDC);
        wglMakeCurrent(dummyHDC, dummyCtx);

        // Load GLAD to get WGL extension functions
        if (!gladLoadGLLoader((GLADloadproc)wglGetProcAddress)) {
            gladLoadGL();
        }

        // wglChoosePixelFormatARB for MSAA
        using PFNWGLCHOOSEPIXELFORMATARBPROC = BOOL (WINAPI *)(HDC, const int*, const float*, UINT, int*, UINT*);
        auto wglChoosePixelFormatARB =
            (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");

        using PFNWGLCREATECONTEXTATTRIBSARBPROC = HGLRC (WINAPI *)(HDC, HGLRC, const int*);
        auto wglCreateContextAttribsARB =
            (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

        // Cleanup dummy
        wglMakeCurrent(nullptr, nullptr);
        wglDeleteContext(dummyCtx);
        ReleaseDC(dummyHWND, dummyHDC);
        DestroyWindow(dummyHWND);
        UnregisterClass("OGLDummy", inst);

        // 2. Create real window
        WNDCLASS wc = {};
        wc.style         = CS_OWNDC;
        wc.lpfnWndProc   = Window::WndProc;
        wc.hInstance     = inst;
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = "OGL33Window";
        RegisterClass(&wc);

        DWORD style = WS_OVERLAPPEDWINDOW;
        RECT rect = {0, 0, w, h};
        AdjustWindowRect(&rect, style, FALSE);

        hwnd = CreateWindow("OGL33Window", title, style,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rect.right - rect.left, rect.bottom - rect.top,
            nullptr, nullptr, inst, this);
        if (!hwnd) return false;
        hdc = GetDC(hwnd);

        // 3. Choose pixel format with MSAA if requested
        int pf = 0;
        if (msaa && wglChoosePixelFormatARB) {
            int samples[] = {8, 4, 2}; // try 8x, then 4x, then 2x
            for (int s = 0; s < 3; s++) {
                int iAttribs[] = {
                    0x2001, 1,           // WGL_DRAW_TO_WINDOW_ARB
                    0x2002, 1,           // WGL_SUPPORT_OPENGL_ARB
                    0x2003, 0,           // WGL_ACCELERATION_ARB → full acceleration
                    0x2010, 1,           // WGL_DOUBLE_BUFFER_ARB
                    0x2011, 1,           // WGL_SAMPLE_BUFFERS_ARB
                    0x2012, samples[s],  // WGL_SAMPLES_ARB
                    0x2013, 32,          // WGL_COLOR_BITS_ARB
                    0x2014, 24,          // WGL_DEPTH_BITS_ARB
                    0x2015, 8,           // WGL_STENCIL_BITS_ARB
                    0x2027, 0x2027,      // WGL_PIXEL_TYPE_ARB → RGBA
                    0, 0
                };
                UINT numFormats = 0;
                float fAttribs[] = {0,0};
                if (wglChoosePixelFormatARB(hdc, iAttribs, fAttribs, 1, &pf, &numFormats) && numFormats > 0) {
                    msaaSamples = samples[s];
                    printf("[WGL] MSAA %dx pixel format chosen (pf=%d)\n", msaaSamples, pf);
                    break;
                }
            }
        }

        if (pf == 0) {
            // Fallback: basic pixel format
            msaaSamples = 0;
            PIXELFORMATDESCRIPTOR pfd = {};
            pfd.nSize = sizeof(pfd);
            pfd.nVersion = 1;
            pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
            pfd.iPixelType = PFD_TYPE_RGBA;
            pfd.cColorBits = 32;
            pfd.cDepthBits = 24;
            pfd.cStencilBits = 8;
            pf = ChoosePixelFormat(hdc, &pfd);
            printf("[WGL] No MSAA — using basic pixel format (pf=%d)\n", pf);
        }

        PIXELFORMATDESCRIPTOR chosenPFD;
        DescribePixelFormat(hdc, pf, sizeof(chosenPFD), &chosenPFD);
        SetPixelFormat(hdc, pf, &chosenPFD);

        // 4. Create OpenGL 3.3 Core context
        if (wglCreateContextAttribsARB) {
            // GLAD needs to be re-loaded for the new context (but wgl funcs are per-module)
            int attribs[] = {
                0x2091, 3,  // WGL_CONTEXT_MAJOR_VERSION_ARB
                0x2092, 3,  // WGL_CONTEXT_MINOR_VERSION_ARB
                0x2094, 0,  // WGL_CONTEXT_FLAGS_ARB
                0x9126, 1,  // WGL_CONTEXT_PROFILE_MASK_ARB → Core
                0, 0
            };
            hglrc = wglCreateContextAttribsARB(hdc, nullptr, attribs);
            if (hglrc) {
                wglMakeCurrent(hdc, hglrc);
                // Reload GLAD for the Core context
                if (!gladLoadGLLoader((GLADloadproc)wglGetProcAddress)) {
                    gladLoadGL();
                }
                printf("[WGL] OpenGL 3.3 Core context created (MSAA=%dx)\n", msaaSamples);
            }
        }

        if (!hglrc) {
            // Legacy fallback
            printf("[WGL] WARNING: Falling back to legacy context\n");
            hglrc = wglCreateContext(hdc);
            wglMakeCurrent(hdc, hglrc);
            if (!gladLoadGLLoader((GLADloadproc)wglGetProcAddress)) {
                gladLoadGL();
            }
        }

        ShowWindow(hwnd, SW_SHOW);
        return true;
    }

    void destroy() {
        if (hglrc) { wglMakeCurrent(nullptr, nullptr); wglDeleteContext(hglrc); hglrc = nullptr; }
        if (hdc)   { ReleaseDC(hwnd, hdc); hdc = nullptr; }
        if (hwnd)  { DestroyWindow(hwnd); hwnd = nullptr; }
    }

    bool pollEvents() {
        scrollDelta = 0;
        resized = false;
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) { shouldClose = true; return false; }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return !shouldClose;
    }

    void swapBuffers() {
        SwapBuffers(hdc);
    }

    void setTitle(const char* title) {
        SetWindowText(hwnd, title);
    }

    double getTime() const {
        LARGE_INTEGER freq, count;
        QueryPerformanceFrequency(&freq);
        QueryPerformanceCounter(&count);
        return (double)count.QuadPart / (double)freq.QuadPart;
    }

    bool isKeyDown(int vk) const { return keys[vk & 0xFF]; }

    static Window* getThis(HWND h) {
        return (Window*)GetWindowLongPtr(h, GWLP_USERDATA);
    }

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        Window* self = getThis(hwnd);

        switch (msg) {
        case WM_CREATE: {
            CREATESTRUCT* cs = (CREATESTRUCT*)lp;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)cs->lpCreateParams);
            return 0;
        }
        case WM_SIZE: {
            int w = LOWORD(lp), h = HIWORD(lp);
            if (self && w > 0 && h > 0) {
                self->width = w; self->height = h;
                self->resized = true;
                if (self->onResize) self->onResize(w, h);
                else glViewport(0, 0, w, h);
            }
            return 0;
        }
        case WM_KEYDOWN:
            if (self) self->keys[wp & 0xFF] = true;
            return 0;
        case WM_KEYUP:
            if (self) self->keys[wp & 0xFF] = false;
            return 0;
        case WM_LBUTTONDOWN:
            if (self) {
                self->mouseDown[0] = true;
                self->mouseDragging = true;
                self->lastMouseX = self->mouseX;
                self->lastMouseY = self->mouseY;
                if (self->onMouseButton) self->onMouseButton(0, 1);
            }
            SetCapture(hwnd);
            return 0;
        case WM_LBUTTONUP:
            if (self) { self->mouseDown[0] = false; self->mouseDragging = false;
                if (self->onMouseButton) self->onMouseButton(0, 0); }
            ReleaseCapture();
            return 0;
        case WM_MOUSEMOVE:
            if (self) {
                self->mouseX = LOWORD(lp);
                self->mouseY = HIWORD(lp);
                if (self->mouseDragging && self->onMouseDrag) {
                    float dx = (float)(self->mouseX - self->lastMouseX);
                    float dy = (float)(self->mouseY - self->lastMouseY);
                    self->onMouseDrag(dx, dy);
                }
                self->lastMouseX = self->mouseX;
                self->lastMouseY = self->mouseY;
            }
            return 0;
        case WM_MOUSEWHEEL:
            if (self) {
                self->scrollDelta = (double)GET_WHEEL_DELTA_WPARAM(wp) / (double)WHEEL_DELTA;
                if (self->onScroll) self->onScroll((float)self->scrollDelta);
            }
            return 0;
        case WM_DESTROY:
            if (self) self->shouldClose = true;
            PostQuitMessage(0);
            return 0;
        case WM_CLOSE:
            if (self) self->shouldClose = true;
            DestroyWindow(hwnd);
            return 0;
        }
        return DefWindowProc(hwnd, msg, wp, lp);
    }
};
