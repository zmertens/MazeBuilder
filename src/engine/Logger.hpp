#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>

class Logger
{
public:
    explicit Logger();

    void appendToLog(const std::string& str);
    void clearLog();
    void dumpLogToFile(const std::string& outFileName) const;

private:
    std::string mLog;
};

#endif // LOGGER_HPP
