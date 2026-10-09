#ifndef VOXELSPIRE_INPUT_MOUSE_CAPTURE_HPP
#define VOXELSPIRE_INPUT_MOUSE_CAPTURE_HPP

#include "../fizmo.hpp"

#if defined(OS_WINDOWS)
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #undef near
    #undef far
#endif

namespace voxelspire {

struct MouseDelta { double dx = 0.0, dy = 0.0; };

class MouseCapture {
public:
    ~MouseCapture() { release(); }

    bool captured() const noexcept { return m_captured; }

    void capture(void* native_handle, const fizmo::windows::InputManager& input) noexcept {
        if (m_captured) return;
        m_handle = native_handle;
        m_captured = true;
        m_last_x = input.mouse_x();
        m_last_y = input.mouse_y();
        m_skip_next = true;

    #if defined(OS_WINDOWS)
        if (HWND hwnd = static_cast<HWND>(m_handle)) {
            while (ShowCursor(FALSE) >= 0) {}
            RECT rc; GetClientRect(hwnd, &rc);
            POINT tl{ rc.left, rc.top }, br{ rc.right, rc.bottom };
            ClientToScreen(hwnd, &tl); ClientToScreen(hwnd, &br);
            RECT clip{ tl.x, tl.y, br.x, br.y };
            ClipCursor(&clip);
            recenter();
        }
    #endif
    }

    void release() noexcept {
        if (!m_captured) return;
        m_captured = false;
    #if defined(OS_WINDOWS)
        ClipCursor(nullptr);
        while (ShowCursor(TRUE) < 0) {}
    #endif
    }

    MouseDelta poll(const fizmo::windows::InputManager& input) noexcept {
        MouseDelta d;
        if (!m_captured) return d;

    #if defined(OS_WINDOWS)
        if (HWND hwnd = static_cast<HWND>(m_handle)) {
            (void)input;
            POINT center = client_center(hwnd);
            POINT cur; GetCursorPos(&cur);
            d.dx = double(cur.x - center.x);
            d.dy = double(cur.y - center.y);
            SetCursorPos(center.x, center.y);
            if (m_skip_next) { m_skip_next = false; return {}; }
            return d;
        }
    #endif

        const int x = input.mouse_x(), y = input.mouse_y();
        d.dx = double(x - m_last_x);
        d.dy = double(y - m_last_y);
        m_last_x = x; m_last_y = y;
        if (m_skip_next) { m_skip_next = false; return {}; }
        return d;
    }

private:
#if defined(OS_WINDOWS)
    static POINT client_center(HWND hwnd) noexcept {
        RECT rc; GetClientRect(hwnd, &rc);
        POINT c{ (rc.right - rc.left) / 2, (rc.bottom - rc.top) / 2 };
        ClientToScreen(hwnd, &c);
        return c;
    }
    void recenter() noexcept {
        if (HWND hwnd = static_cast<HWND>(m_handle)) { POINT c = client_center(hwnd); SetCursorPos(c.x, c.y); }
    }
#endif

    void* m_handle    = nullptr;
    bool  m_captured  = false;
    bool  m_skip_next = false;
    int   m_last_x    = 0;
    int   m_last_y    = 0;
};

} // namespace voxelspire

#endif // VOXELSPIRE_INPUT_MOUSE_CAPTURE_HPP
