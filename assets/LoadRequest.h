#pragma once
#include <string>
#include <functional>

namespace assets {

    // �ɰ�����չ������ö�١����ȼ���
    struct LoadRequest {
        std::string path;
        std::function<void()> work; // ��ִ̨�е����񣨷�װ�˽���/�������裩
    };

} // namespace assets
