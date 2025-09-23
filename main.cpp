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
            };

            const std::vector<uint16_t> indices = {
                // Left square
                0,
                1,
                2,
                0,
                2,
                3,
            };

            // _VB = VEngine::VertexBuffer::Create((float *)vertices.data(), 5 * vertices.size(), BufferTypes::STATIC);
            // _IB = VEngine::IndexBuffer::Create((void *)indices.data(), indices.size(), IndexBufferType::UINT_16, BufferTypes::STATIC);
        }

        void OnUpdate(TimeStep ts) override
        {
            // TODO: Rename 
            RenderPassSpec Spec;
            Spec.ClearColor = {0.2, 0.2, 0.2, 1.0f};

            Renderer::Begin(Spec);
            Renderer::Submit();
            Renderer::End();

            Renderer::Render(); // to a framebuffer
            Renderer::Present();
        }

        void OnTerminate() override
        {
            // _VB->Destroy();
            // _IB->Destroy();
        }

        void OnEvent(Event &e) override
        {
            if (e.GetType() == WindowResizeEvent::GetStaticType())
                VENGINE_APP_PRINTLN("Window Resize")
        }

    private:
        std::shared_ptr<VEngine::VertexBuffer> _VB;
        std::shared_ptr<VEngine::IndexBuffer> _IB;
        std::shared_ptr<VEngine::Shader> _Shader;
    };
}; // namespace VEngine

int main(int argc, char const *argv[])
{
    VEngine::ApplicationSpec Spec;
    Spec.Name = "VEngine Editor";
    Spec.Dimensions = {1200.0f, 780.0f};
    Spec.VSync = false;

    VEngine::Application app;
    app.OnInit(Spec);
    app.PushLayer(std::make_shared<VEngine::Editor>());

    app.OnUpdate();
    app.OnTerminate();

    return 0;
}
