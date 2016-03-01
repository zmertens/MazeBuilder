#include "StateMap.hpp"

/**
 * @brief StateMap::StateMap
 */
StateMap::StateMap()
: mStates()
{

}

/**
 * @brief StateMap::find
 * @param key
 * @return
 */
const IState::Ptr& StateMap::find(int key) const
{
    const auto& state = mStates.at(key);

    if (state != nullptr)
    {
        return state;
    }
    else
    {
        throw new std::logic_error("\nState does not exist in State Map\n");
    }
}

/**
 * @brief StateMap::push
 * @param key
 * @param state
 */
void StateMap::push(int key, IState::Ptr state)
{
    mStates[key] = std::move(state);
}

/**
 * @brief StateMap::remove
 * @param key
 */
void StateMap::remove(int key)
{
    auto& state = mStates.at(key);

    if (state != nullptr)
    {
        IState* statePtr = state.release();
        delete statePtr;
        statePtr = nullptr;
    }
    else
    {
        throw new std::logic_error("\nState does not exist in State Map\n");
    }
}

