#ifndef DATALOGGER_HPP
#define DATALOGGER_HPP

#include <string>

class DataLogger
{
public:
    explicit DataLogger();

    void appendToLog(const std::string& str);
    void clearLog();
    void dumpLogToFile(const std::string& outFileName) const;

private:
    std::string mLog;
};

#endif // DATALOGGER_HPP
