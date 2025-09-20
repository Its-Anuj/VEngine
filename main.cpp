#include "VePCH.h"
#include "Application.h"

namespace VEngine
{
    struct Vertex
    {
        Vec2 pos;
        Vec3 color;
    };

    class Editor : public Layer
    {
    public:
        Editor()
            : Layer("Editor")
        {
        }
        ~Editor()
        {
        }

        void OnInit() override
        {
            const std::vector<Vertex> vertices = {
                // Left square (center x=-0.5)
                {{-0.75f, -0.25f}, {1.0f, 0.0f, 0.0f}}, // 0: bottom-left, red
                {{-0.25f, -0.25f}, {0.0f, 1.0f, 0.0f}}, // 1: bottom-right, green
                {{-0.25f, 0.25f}, {0.0f, 0.0f, 1.0f}},  // 2: top-right, blue
                {{-0.75f, 0.25f}, {1.0f, 1.0f, 0.0f}},  // 3: top-left, yellow

                // Right square (center x=0.5)
                {{0.25f, -0.25f}, {1.0f, 0.0f, 1.0f}}, // 4: bottom-left, magenta
                {{0.75f, -0.25f}, {0.0f, 1.0f, 1.0f}}, // 5: bottom-right, cyan
                {{0.75f, 0.25f}, {1.0f, 1.0f, 1.0f}},  // 6: top-right, white
                {{0.25f, 0.25f}, {0.5f, 0.5f, 0.5f}}   // 7: top-left, gray
            };

            const std::vector<Vertex> ChangingVertices = {
                // Top quad (centered at y = +0.6)
                {{-0.25f, 0.35f}, {1.0f, 0.0f, 0.0f}}, // 0: bottom-left
                {{0.25f, 0.35f}, {0.0f, 1.0f, 0.0f}},  // 1: bottom-right
                {{0.25f, 0.85f}, {0.0f, 0.0f, 1.0f}},  // 2: top-right
                {{-0.25f, 0.85f}, {1.0f, 1.0f, 0.0f}}, // 3: top-left

                // Bottom quad (centered at y = -0.6)
                {{-0.25f, -0.85f}, {1.0f, 0.0f, 1.0f}}, // 4: bottom-left
                {{0.25f, -0.85f}, {0.0f, 1.0f, 1.0f}},  // 5: bottom-right
                {{0.25f, -0.35f}, {1.0f, 1.0f, 1.0f}},  // 6: top-right
                {{-0.25f, -0.35f}, {0.5f, 0.5f, 0.5f}}  // 7: top-left
            };

            const std::vector<uint16_t> indices = {
                // Left square
                0, 1, 2,
                0, 2, 3,

                // Right square
                4, 5, 6,
                4, 6, 7};

            _VB = VEngine::VertexBuffer::Create((float *)vertices.data(), 5 * vertices.size());
            _VB2 = VEngine::VertexBuffer::Create((float *)ChangingVertices.data(), 5 * ChangingVertices.size());
            _IB = VEngine::IndexBuffer::Create((void *)indices.data(), indices.size(), IndexBufferType::UINT_16);
        }

        void OnUpdate(TimeStep ts) override
        {
            RenderPassSpec Spec;
            Spec.ClearColor = {0.2, 0.2, 0.2, 1.0f};

            Renderer::Begin(Spec);
            Renderer::Submit(_Shader, _VB, _IB);
            Renderer::Submit(_Shader, _VB2, _IB);
            Renderer::End();

            Renderer::Render(); // to a framebuffer
            Renderer::Present();
        }

        void OnTerminate() override
        {
            _VB->Destroy();
            _VB2->Destroy();
            _IB->Destroy();
        }

        void OnEvent(Event &e) override
        {
            if (e.GetType() == WindowResizeEvent::GetStaticType())
                VENGINE_APP_PRINTLN("Window Resize")
        }

    private:
        std::shared_ptr<VEngine::VertexBuffer> _VB, _VB2;
        std::shared_ptr<VEngine::IndexBuffer> _IB;
        std::shared_ptr<VEngine::Shader> _Shader;
    };
}; // namespace VEngine

int main(int argc, char const *argv[])
{
    VEngine::ApplicationSpec Spec;
    Spec.Name = "VEngine Editor";
    Spec.Dimensions = {1200.0f, 780.0f};
    Spec.VSync = true;

    VEngine::Application app;
    app.OnInit(Spec);
    app.PushLayer(std::make_shared<VEngine::Editor>());

    app.OnUpdate();
    app.OnTerminate();

    return 0;
}
