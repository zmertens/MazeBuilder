#ifndef COMMAND_QUEUE_H
#define COMMAND_QUEUE_H

#include <queue>

#include "command.h"

class command_queue
{
public:
    void push(const command &command)
    {
        commands.push(command);
    }

    command pop()
    {
        command cmd = commands.front();
        commands.pop();
        return cmd;
    }

    bool is_empty() const
    {
        return commands.empty();
    }

private:
    std::queue<command> commands;
};

    #endif // COMMAND_QUEUE_H