#include "CommandQueue.hpp"

void CommandQueue::push(const Command& command)
{
    commands.push(command);
}

Command CommandQueue::pop()
{
    Command cmd = commands.front();
    commands.pop();
    return cmd;
}

bool CommandQueue::isEmpty() const
{
    return commands.empty();
}
