// ============================================================
// xyplot/iinput_source.h — 输入事件抽象接口 (冻结 v1.0)
// ============================================================
// IInputSource 为完全可选。客户不实现时 SDK 仅提供渲染能力。
// ============================================================
#pragma once

namespace xyplot {

struct InputEvent {
    enum Type {
        None = 0,
        MouseDown, MouseUp, MouseMove, MouseWheel,
        KeyDown, KeyUp,
        TouchBegin, TouchMove, TouchEnd
    };
    Type type = None;
    double x = 0, y = 0;
    int button = 0;
    int modifiers = 0;
    double wheelDelta = 0;
    int keyCode = 0;
};

class IInputSource {
public:
    virtual ~IInputSource() = default;
    virtual bool pollEvent(InputEvent& out) = 0;
};

} // namespace xyplot
