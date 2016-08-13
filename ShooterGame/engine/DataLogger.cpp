#include "DataLogger.hpp"

#include <fstream>

/**
 * @brief DataLogger::DataLogger
 */
DataLogger::DataLogger()
: mLog("DataLogger Begin:\n")
{

}

/**
 * @brief DataLogger::appendToLog
 * @param str
 */
void DataLogger::appendToLog(const std::string& str)
{
    mLog += str;
}

/**
 * @brief DataLogger::clearLog
 */
void DataLogger::clearLog()
{
    mLog.clear();
}

/**
 * @brief DataLogger::dumpLogToFile
 * @param outFileName
 */
void DataLogger::dumpLogToFile(const std::string& outFileName) const
{
    std::ofstream outStream;
    outStream.open(outFileName);
    outStream << mLog;
    outStream.close();
}
