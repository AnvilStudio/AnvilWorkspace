
#include "../Scene.h"

namespace anv
{
    class Scene2D
        : public Scene
    {
    public:
        Scene2D(std::string _name);

        void Init()     override;
        void Shutdown() override;
        void OnUpdate(float _deltaTime) override;
        void Render()   override;

        entt::entity CreateEntity(std::string _tag) override;
        void DestroyEntity(entt::entity _ent) override;

    private:
        Camera2D m_Camera;
    };
}