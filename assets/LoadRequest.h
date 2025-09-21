#pragma once
#include <string>
#include <functional>

namespace assets {

    // 可按需扩展：类型枚举、优先级等
    struct LoadRequest {
        std::string path;
        std::function<void()> work; // 后台执行的任务（封装了解析/构建步骤）
    };

} // namespace assets
