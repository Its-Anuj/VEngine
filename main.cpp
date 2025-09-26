#include "VePCH.h"
#include "Application.h"

namespace VEngine
{

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
            std::vector<Vertex> vertices = {
                // pos         // color            // tex
                {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}}, // bottom-left
                {{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},  // bottom-right
                {{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},   // top-right
                {{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},  // top-left
            };

            std::vector<uint16_t> indicies = {
                0, 1, 2, // first triangle
                2, 3, 0  // second triangle
            };
            {
                VertexBufferDesc desc{};
                desc.Data = (void *)vertices.data();
                desc.SizeInBytes = sizeof(Vertex) * vertices.size();
                desc.Type = BufferTypes::STATIC_DRAW;
                desc.Count = vertices.size();

                _TriangleVB = Renderer::__GetResouceFactory()->CreateVertexBuffer(desc);
            }
            {
                IndexBufferDesc desc{};
                desc.Data = (void *)indicies.data();
                desc.SizeInBytes = sizeof(uint16_t) * indicies.size();
                desc.Type = BufferTypes::STATIC_DRAW;
                desc.Count = indicies.size();
                desc.DataType = IndexBufferType::UINT_16;

                _TriangleIB = Renderer::__GetResouceFactory()->CreateIndexBuffer(desc);
            }
        }

        void OnUpdate(TimeStep ts) override
        {
            VENGINE_APP_PRINTLN("Fps:  " << ts.GetFPS() << " Time: " << ts.GetMilliSecond());

            // TODO: Rename
            RenderPassSpec Spec;
            Spec.ClearColor = {0.2, 0.2, 0.2, 1.0f};

            Renderer::Begin(Spec);
            Renderer::Submit(_TriangleVB, _TriangleIB);
            Renderer::End();

            Renderer::Render(); // to a framebuffer
            Renderer::Present();
        }

        void OnTerminate() override
        {
            Renderer::Finish();
            Renderer::__GetResouceFactory()->DeleteVertexBuffer(_TriangleVB);
            Renderer::__GetResouceFactory()->DeleteIndexBuffer(_TriangleIB);
        }

        void OnEvent(Event &e) override
        {
            if (e.GetType() == WindowResizeEvent::GetStaticType())
                VENGINE_APP_PRINTLN("Window Resize")
        }

    private:
        Ref<VertexBuffer> _TriangleVB;
        Ref<IndexBuffer> _TriangleIB;
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
