#ifndef TSUPFDETMONMESSAGE_H
#define TSUPFDETMONMESSAGE_H

#include <string>

class TSupFDetMonMessage
{
public:
    TSupFDetMonMessage() = default;
    explicit TSupFDetMonMessage(std::string command);

    const std::string& GetCommand() const noexcept;
    void SetCommand(std::string command);

private:
    std::string fCommand;
};

#endif
