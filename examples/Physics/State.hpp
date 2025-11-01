#ifndef STATE_HPP
#define STATE_HPP

#include <memory>

#include "RenderWindow.hpp"
#include "ResourceIdentifiers.hpp"
#include "StateIdentifiers.hpp"

class Player;
class StateStack;
union SDL_Event;

class State {
public:
    typedef std::unique_ptr<State> Ptr;
    
    struct Context { 

        explicit Context(RenderWindow& window, TextureManager& textures, Player& player);

        RenderWindow* window;
        TextureManager* textures;  
        Player* player;
    };
public:
    explicit State(StateStack& stack, Context context);

    virtual ~State() = default;

    // Delete copy constructor and copy assignment operator
    // because State contains std::unique_ptr which is not copyable
    State(const State&) = delete;
    State& operator=(const State&) = delete;

    // Allow move constructor and move assignment operator
    State(State&&) = default;
    State& operator=(State&&) = default;

    virtual void draw() const noexcept = 0;
    virtual bool update(float dt) noexcept = 0;
    virtual bool handleEvent(const SDL_Event& event) noexcept = 0;
protected:
    void requestStackPush(States::ID stateID);
    void requestStackPop();
    void requestStateClear();
    Context getContext() const noexcept;
private:
    StateStack* mStack;
    Context mContext;
};
#endif // STATE_HPP
