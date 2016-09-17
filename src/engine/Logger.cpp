#include "Logger.hpp"

#include <fstream>

/**
 * @brief Logger::Logger
 */
Logger::Logger()
: mLog("Logger Begin:\n")
{

}

/**
 * @brief Logger::appendToLog
 * @param str
 */
void Logger::appendToLog(const std::string& str)
{
    mLog += str;
}

/**
 * @brief Logger::clearLog
 */
void Logger::clearLog()
{
    mLog.clear();
}

/**
 * @brief Logger::dumpLogToFile
 * @param outFileName
 */
void Logger::dumpLogToFile(const std::string& outFileName) const
{
    std::ofstream outStream;
    outStream.open(outFileName);
    outStream << mLog;
    outStream.close();
}
