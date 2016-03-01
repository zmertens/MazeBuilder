#ifndef STATEMAP_HPP
#define STATEMAP_HPP

#include <array>

#include "TitleState.hpp"
#include "MenuState.hpp"
#include "PlayState.hpp"

class StateMap
{
public:
    enum class Type {
        TITLE,
        MENU,
        PLAY,
        TOTAL_STATES
    };
public:
    explicit StateMap();

    const IState::Ptr& find(int key) const;
    void push(int key, IState::Ptr state);
    void remove(int key);

private:
    std::array<IState::Ptr, static_cast<int>(Type::TOTAL_STATES)> mStates;
};

#endif // STATEMAP_HPP
