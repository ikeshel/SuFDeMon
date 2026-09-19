#include "TSupFDetMonMessage.h"

#include <utility>

TSupFDetMonMessage::TSupFDetMonMessage(std::string command)
    : fCommand(std::move(command))
{
}

const std::string& TSupFDetMonMessage::GetCommand() const noexcept
{
    return fCommand;
}

void TSupFDetMonMessage::SetCommand(std::string command)
{
    fCommand = std::move(command);
}
