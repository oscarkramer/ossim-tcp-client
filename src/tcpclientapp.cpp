#include "tcpclientapp.h"

#include <future>
#include <iomanip>


namespace utils {

std::mutex Timing::outMutex_;

Timing::Timing(const std::string &title)
    : title_(title),
      t_start_(std::chrono::high_resolution_clock::now()),
      is_out_result_(false)
{
    std::time_t t = std::time(nullptr);
    //    std::locale::global(std::locale());
    char date_time[100];
    if (std::strftime(date_time, sizeof(date_time), "%x %X", std::localtime(&t))) {
        outMutex_.lock();
        std::cout << title_ << " is started at " << date_time << std::endl;
        outMutex_.unlock();
    }
}

Timing::~Timing()
{
    if (!is_out_result_) {
        outResultStr();
    }
}

void Timing::outResultStr(const int64_t bytes)
{
    is_out_result_ = true;
    auto t_end = std::chrono::high_resolution_clock::now();
    auto t_value = std::chrono::duration<double, std::milli>(t_end - t_start_).count();
    auto speedByteSec = bytes * 1000 / t_value;
    std::string bitsStr(to_freindly_string(speedByteSec * 8));
    utils::replace_substr(bitsStr, "B", "bit");

    outMutex_.lock();
    std::cout << title_ << " finished: " << to_freindly_string(bytes) << " for " << t_value / 1000 << " seconds ("
              << bitsStr << "/s)" << std::endl;
    outMutex_.unlock();
}

void Timing::outResultStr()
{
    is_out_result_ = true;
    auto t_end = std::chrono::high_resolution_clock::now();
    auto t_value = std::chrono::duration<double, std::milli>(t_end - t_start_).count();
    outMutex_.lock();
    std::cout << title_ << " finished: " << utils::num_to_string(t_value) << " ms. = " << utils::num_to_string(t_value / 1000) <<
              " sec. "
              << std::endl;
    outMutex_.unlock();
}

void Timing::reStart()
{
    t_start_ = std::chrono::high_resolution_clock::now();
}

void Timing::outStr(const std::string &str)
{
    TCPClient::out_str(str, std::cout);
//    outMutex_.lock();
//    std::cout << str << std::endl;
//    outMutex_.unlock();
}

void Timing::sleepMs(const int sleepMs)
{
#ifdef _WIN32
    Sleep(static_cast<uint16_t>(sleepMs));
#else
    usleep(sleepMs * 1000);
#endif
}

long long Timing::runTimesNano()
{
    return (std::chrono::high_resolution_clock::now() - t_start_).count();
}

StdInOutHandler::StdInOutHandler(std::string &inFile, std::string &outFile) : fileNameIn_(""), fileNameOut_(""), error_(0),
    prevCoutBuf_(std::cout.rdbuf())
{
    if ("STDIN" == utils::str_to_upper(inFile)) {
        inFile = "stdin.tmp";
        fileNameIn_ = inFile;
    }
    if ("STDOUT" == utils::str_to_upper(outFile)) {
        outFile = "stdout.tmp";
        fileNameOut_ = outFile;
    }
    start();
}

void StdInOutHandler::start()
{
    if (!fileNameIn_.empty()) {
        std::ofstream ofs;
        ofs.open(fileNameIn_, std::ofstream::out | std::ios::binary);
        if (!ofs.is_open()) {
            std::cerr << "File \"" << fileNameIn_ << "\" not created." << std::endl;
            error_ = 2;
        }
        if (0 == error_) {
            ofs << std::cin.rdbuf();
        }
    }
    if (0 == error_) {
        if (!fileNameOut_.empty()) {
            std::cout.rdbuf(tmpCoutStream_.rdbuf());
        }
    }
}

StdInOutHandler::~StdInOutHandler()
{
    if (!fileNameOut_.empty()) {
        std::ifstream ifs(fileNameOut_, std::ios::binary);
        if (!ifs) {
            std::cerr << "File \"" << fileNameOut_ << "\" not found.\n";
            error_ = 2;
        }
        if (0 == error_) {
            std::cout.rdbuf(prevCoutBuf_);
            std::cout << ifs.rdbuf();
            ifs.close();
            remove(fileNameOut_.c_str());
        }
    }
    if (!fileNameIn_.empty()) {
        remove(fileNameIn_.c_str());
    }
}

bool compareHexFiles(const std::string &f1, const std::string &f2)
{
    std::ifstream ifs1(f1);
    std::ifstream ifs2(f2);
    bool result = true;
    if (!ifs1.is_open()) {
        std::cout << "Error to open file: " << f1;
        result = false;
    }
    if (!ifs2.is_open()) {
        std::cout << "Error to open file: " << f1;
        result = false;
    }
    uint64_t v1 = 0;
    uint64_t v2 = 0;
    ifs1 >> std::hex;
    ifs2 >> std::hex;
    while (ifs1 >> v1 && ifs2 >> v2) {
        if (v1 != v2) {
            result = false;
            break;
        }
    }
    ifs1.close();
    ifs2.close();
    return result;
}


namespace convertors {

namespace ds8binHeader {

long long size()
{
    return sizeof(DS8_FILE_HEADER);
}
bool write(std::ofstream &ofs, const uint32_t blockSize)
{
    DS8_FILE_HEADER header;
    memcpy(header.fields.title, DS8_HEADER_TITLE, strlen(DS8_HEADER_TITLE));
    header.fields.blockSize = blockSize;
    ofs.write(header.buf, size());
    return true;
}
bool read(std::ifstream &ifs, uint32_t &blockSize)
{
    ifs.seekg(std::ios::beg);
    DS8_FILE_HEADER header;
    ifs.read(header.buf, size());
    if (0 == strcmp(header.fields.title, DS8_HEADER_TITLE)) {
        blockSize = header.fields.blockSize;
        return true;
    }
    return false;
}
} // namespace ds8binHeader


std::ifstream::pos_type getFileSize(const std::string &filename)
{
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return in.good() ? in.tellg() : std::ifstream::pos_type(0);
}

bool file_bin_to_ds8(const std::string &fileName, const std::string &fileNameDs8, const uint32_t bufSizeB)
{
    std::ifstream ifs(fileName, std::ios::binary);
    if (!ifs) {
        std::cerr << "File \"" << std::string(fileName) << "\" not found.\n";
        return false;
    }
    std::ofstream ofs(fileNameDs8, std::ofstream::out | std::ios::binary);
    if (!ofs.is_open()) {
        std::cerr << "File \"" << std::string(fileNameDs8) << "\" cannot be created.\n";
        ifs.close();
        return false;
    }

    std::cout << "File " << fileName << " converting to " << fileNameDs8 << "...\n";
    if (!ds8binHeader::write(ofs, bufSizeB)) {
        std::cerr << "File header write error: " << GetLastError() << "." << std::endl;
        ifs.close();
        ofs.close();
        return false;
    }
    auto fileSize = getFileSize(fileName);
    std::cout << "Use size of block: " << bufSizeB << "\n";
    std::vector<DS8WORD> data(bufSizeB / sizeof(uint64_t));
    auto pChar = data[0].c_8;
    auto pDataChar = data[2].c_8;
    auto headerSize = 2 * sizeof(uint64_t);
    data[0].w_64 = bufSizeB - headerSize;
    data[1].w_64 = DataTypes::data;
    size_t countW(0);
    int64_t readBytes(0);
    for (; readBytes < fileSize;) {
        auto needBytes = std::min<int64_t>(fileSize - readBytes, bufSizeB - headerSize);
        if (needBytes <= 0) {
            break;
        }
        ifs.read(pDataChar, needBytes);
        if (!ifs.good()) {
            if (!ifs.good()) {
                std::cerr << "Read from file error: " << GetLastError() << "\n";
                return false;

            }
        }
        readBytes += needBytes;
        if (fileSize == readBytes) { //last block of data
            auto i = static_cast<size_t>(needBytes + 7) / 8 + 2;
            data[0].w_64 = needBytes;
            for (auto j = i; j < data.size(); j++) {
                data[j].w_64 = 0x2020202020202020;
            }
        }
        ofs.write(pChar, bufSizeB);
        if (!ofs.good()) {
            std::cerr << "Write to file error: " << GetLastError() << "\n";
            return false;
        }
        countW += data.size() - 2;

        if (countW % 100000 == 0) {

            std::cout << "\rWords is convert: " << countW;
        }
    }
    std::cout << "\rAll words is convert: " << countW << std::endl;
    ifs.close();
    for (auto &val : data) {
        val.w_64 = 0;
    }
    data[0].w_64 = 8;
    data[1].w_64 = DataTypes::service;
    data[2].w_64 = ControlTags::terminate;
    ofs.write(pChar, bufSizeB);

    ofs.close();

    return true;
}


bool file_hex_to_ds8(const std::string &fileNameHex, const std::string &fileNameBin, const uint32_t bufSizeB)
{
    std::ifstream ifs(fileNameHex, std::ios::binary);
    if (!ifs) {
        std::cerr << "File \"" << std::string(fileNameHex) << "\" not found.\n";
        return false;
    }
    std::ofstream ofs(fileNameBin, std::ofstream::out | std::ios::binary);
    if (!ofs.is_open()) {
        std::cerr << "File \"" << std::string(fileNameBin) << "\" cannot be created.\n";
        ifs.close();
        return false;
    }

    std::cout << "File " << fileNameHex << " converting to " << fileNameBin << "...\n";
    if (!ds8binHeader::write(ofs, bufSizeB)) {
        std::cerr << "File header write error: " << GetLastError() << "." << std::endl;
        ifs.close();
        ofs.close();
        return false;
    }
    std::cout << "Use size of block: " << bufSizeB << "\n";
    std::vector<uint64_t> data(bufSizeB / sizeof(uint64_t));
    auto pData = reinterpret_cast<char *>(&data[0]);
    auto headerSize = 2 * sizeof(uint64_t);
    data[0] = bufSizeB - headerSize;
    data[1] = DataTypes::data;
    size_t countW(0);
    ifs >> std::hex;
    bool exit(false);
    for (auto countBlocks = 0; !exit; countBlocks++) {
        size_t i;
        for (i = 2; i < data.size(); i++) {
            if (ifs >> data[i]) {
                countW++;
            } else {
                exit = true;
                break;
            }
        }
        if (data.size() > i) { //last block of data
            data[0] = i * 8 - headerSize;
            for (auto j = i; j < data.size(); j++) {
                data[j] = 0;
            }
        }
        ofs.write(pData, static_cast<long long>(bufSizeB));
        if (!ofs.good()) {
            std::cerr << "Write to file error: " << GetLastError() << "\n";
            return false;
        }

        if (countW % 100000 == 0) {
            std::cout << "\rHex words is convert: " << countW;
        }
    }
    std::cout << "\rAll hex words is convert: " << countW << std::endl;
    ifs.close();
    for (auto &val : data) {
        val = 0;
    }
    data[0] = 8;
    data[1] = DataTypes::service;
    data[2] = ControlTags::terminate;
    ofs.write(reinterpret_cast<char *>(&data[0]), static_cast<long long>(bufSizeB));

    ofs.close();

    return true;
}

bool file_hex_to_bin(const std::string &fileNameHex, const std::string &fileNameBin)
{
    std::ifstream ifs(fileNameHex, std::ios::binary);
    if (!ifs) {
        std::cerr << "File \"" << std::string(fileNameHex) << "\" not found.\n";
        return false;
    }
    std::ofstream ofs(fileNameBin, std::ofstream::out | std::ios::binary);
    if (!ofs.is_open()) {
        std::cerr << "File \"" << std::string(fileNameBin) << "\" cannot be created.\n";
        ifs.close();
        return false;
    }

    std::cout << "File " << fileNameHex << " converting to " << fileNameBin << "...\n";
    auto bufSizeB = 128 * 128;
    std::vector<uint64_t> data(bufSizeB / sizeof(uint64_t));
    auto pDataChar = reinterpret_cast<char *>(&data[0]);
    size_t countW(0);
    ifs >> std::hex;
    bool exit(false);
    for (auto countBlocks = 0; !exit; countBlocks++) {
        size_t i;
        for (i = 0; i < data.size(); i++) {
            if (ifs >> data[i]) {
                countW++;
            } else {
                exit = true;
                break;
            }
        }
        ofs.write(pDataChar, static_cast<long long>(i * 8));
        if (!ofs.good()) {
            std::cerr << "Write to file error: " << GetLastError() << "\n";
            return false;
        }

        if (countW % 100000 == 0) {
            std::cout << "\rHex words is convert: " << countW;
        }
    }
    std::cout << "\rAll hex words is convert: " << countW << std::endl;
    ifs.close();
    ofs.close();

    return true;
}

bool file_bin_to_hex(const std::string &fileName, const std::string &fileNameHex, const bool tag/* = false*/)
{
    std::ifstream ifs(fileName, std::ios::binary);
    if (!ifs) {
        std::cerr << "File \"" << std::string(fileName) << "\" not found.\n";
        return false;
    }

    std::ofstream ofs(fileNameHex, std::ofstream::out | std::ios::binary);
    if (!ofs.is_open()) {
        ifs.close();
        std::cerr << "File \"" << std::string(fileNameHex) << "\" cannot be created .\n";
        return false;
    }

    auto fileSize = getFileSize(fileName);
    const std::string endLine = tag ? " 02\n" : "\n";

    std::cout << "File " << fileName << " converting to " << fileNameHex << "...\n";

    size_t bufSizeB = 128 * 128;
    int64_t readBytes(0);
    size_t countW(0);
    std::vector<uint64_t> data(bufSizeB / 8, 0);
    auto pCharData = reinterpret_cast<char *>(&data[0]);
    ofs << std::hex;
    for (; readBytes < fileSize;) {
        auto needBytes = std::min<int64_t>(bufSizeB, fileSize - readBytes);
        ifs.read(pCharData, needBytes);
        if (!ifs.good()) {
            std::cerr << "Read from file error: " << GetLastError() << "\n";
            return false;
        }
        readBytes += needBytes;
        for (auto j = 0; j < (needBytes + 7) / 8; j++) {
            ofs << std::setw(16) << std::setfill('0') << data[j] << endLine;
            if (!ofs.good()) {
                std::cerr << "Write to file error: " << GetLastError() << "\n";
                return false;
            }
            countW++;
        }
        if (countW % 100000 == 0) {
            std::cout << "\rHex words is convert: " << countW;
        }
    }
    std::cout << "\rAll hex words is convert: " << countW << std::endl;

    ifs.close();
    ofs.close();

    return true;
}


bool file_ds8_to_hex(const std::string &fileName, const std::string &fileNameHex, const bool tag /*= false*/)
{
    std::ifstream ifs(fileName, std::ios::binary);
    if (!ifs) {
        std::cerr << "File \"" << std::string(fileName) << "\" not found.\n";
        return false;
    }
    auto fileSize = getFileSize(fileName);
    if (fileSize < ds8binHeader::size()) {
        ifs.close();
        std::cerr << "Wrong size of file.\n";
        return false;
    }
    fileSize -= ds8binHeader::size();
    uint32_t bufSizeB(1);
    if (!ds8binHeader::read(ifs, bufSizeB)) {
        ifs.close();
        std::cerr << "Wrong type of file.\n";
        return false;
    }
    if (fileSize % bufSizeB != 0) {
        ifs.close();
        std::cerr << "Wrong size of file.\n";
        return false;
    }

    std::ofstream ofs(fileNameHex, std::ofstream::out | std::ios::binary);
    if (!ofs.is_open()) {
        ifs.close();
        std::cerr << "File \"" << std::string(fileNameHex) << "\" cannot be created .\n";
        return false;
    }

    const std::string endLine = tag ? " 02\n" : "\n";

    std::cout << "File " << fileName << " converting to " << fileNameHex << "...\n";

    size_t countW(0);
    auto nBlocks = fileSize / bufSizeB;
    std::vector<uint64_t> data(bufSizeB / 8, 0);
    auto pCharData = reinterpret_cast<char *>(&data[0]);
    ofs << std::hex;
    for (auto i = 0; i < nBlocks - 1; i++) {
        ifs.read(pCharData, static_cast<int64_t>(bufSizeB));
        if (!ifs.good()) {
            std::cerr << "Read from file error: " << GetLastError() << "\n";
            return false;
        }
        for (size_t j = 0; j < data[0] / 8; j++) {
            ofs << std::setw(16) << std::setfill('0') << data[j + 2] << endLine;

            if (!ofs.good()) {
                std::cerr << "Write to file error: " << GetLastError() << "\n";
                return false;
            }
            countW++;
        }
        if (countW % 100000 == 0) {
            std::cout << "\rHex words is convert: " << countW;
        }
    }
    std::cout << "\rAll hex words is convert: " << countW << std::endl;

    ofs.close();
    ifs.close();

    return true;
}

bool file_ds8_to_bin(const std::string &fileName, const std::string &fileNameBin)
{
    std::ifstream ifs(fileName, std::ios::binary);
    if (!ifs) {
        std::cerr << "File \"" << std::string(fileName) << "\" not found.\n";
        return false;
    }
    auto fileSize = getFileSize(fileName);
    if (fileSize < ds8binHeader::size()) {
        ifs.close();
        std::cerr << "Wrong size of file.\n";
        return false;
    }
    fileSize -= ds8binHeader::size();
    uint32_t bufSizeB(1);
    if (!ds8binHeader::read(ifs, bufSizeB)) {
        ifs.close();
        std::cerr << "Wrong type of file.\n";
        return false;
    }
    if (fileSize % bufSizeB != 0) {
        ifs.close();
        std::cerr << "Wrong size of file.\n";
        return false;
    }

    std::ofstream ofs(fileNameBin, std::ofstream::out | std::ios::binary);
    if (!ofs.is_open()) {
        ifs.close();
        std::cerr << "F = falseile \"" << std::string(fileNameBin) << "\" cannot be created .\n";
        return false;
    }

    std::cout << "File " << fileName << " converting to " << fileNameBin << "...\n";

    size_t countW(0);
    auto nBlocks = fileSize / bufSizeB;
    std::vector<DS8WORD> data(bufSizeB);
    auto pChar = data[0].c_8;
    auto pCharData = pChar + 16;
    for (auto i = 0; i < nBlocks - 1; i++) {
        ifs.read(pChar, static_cast<int64_t>(bufSizeB));
        if (!ifs.good()) {
            std::cerr << "Read from file error: " << GetLastError() << "\n";
            return false;
        }
        ofs.write(pCharData, data[0].w_64);
        if (!ofs.good()) {
            std::cerr << "Write to file error: " << GetLastError() << "\n";
            return false;
        }
        countW += data.size() - 2;
        if (countW % 100000 == 0) {
            std::cout << "\rWords is convert: " << countW;
        }
    }
    std::cout << "\rAll words is convert: " << countW << std::endl;

    ofs.close();
    ifs.close();

    return true;
}

uint64_t normalizeBufSize(const uint64_t blockSize)
{
    const uint64_t multiplier = 128;
    const uint64_t maxBlockSize = 65536;
    uint64_t bufSize = std::min<uint64_t>(maxBlockSize, std::max<uint64_t>(multiplier, blockSize));
    unsigned int mod128 = bufSize % multiplier;
    if (mod128 > 0) {
        bufSize += multiplier - mod128;
        std::cout << "Wrong Buffer size. Set to " << bufSize << "\n";
    }

    return bufSize;
}

} // namespace convertors

std::string get_send_speed_msg(TCPClient &client)
{
    auto t_ms = std::chrono::duration<double, std::milli>(client.get_sent_time_ms());
    auto ms = t_ms.count();
//    auto ms = client.get_sent_time_ms();
    if (0 == ms) {
        return "";
    }
    auto bytes = client.get_bytes_sent();
    auto speedByteSec = bytes * 1000 / ms;
    std::string res = "Network speed of sending: " + to_freindly_string(speedByteSec) + "/s";
    return res;
}

std::string get_receive_speed_msg(TCPClient &client)
{
    auto ms = client.get_received_time_ms();
    if (0 == ms) {
        return "";
    }
    auto bytes = client.get_bytes_received();
    auto speedByteSec = bytes * 1000 / ms;
    std::string res = "Network speed of receiving: " + to_freindly_string(speedByteSec) + "/s";
    return res;
}

bool replace_substr(std::string &str, const std::string &from, const std::string &to)
{
    size_t start_pos = str.find(from);
    if (start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

std::string str_to_upper(const std::string &strIn)
{
    std::string strOut("");
    std::transform(strIn.begin(), strIn.end(), std::back_inserter(strOut), toupper);

    return strOut;
}

int status_out(std::vector<std::future<uint64_t>> &f_vec, TCPClient &client, const long long interval/* = 5*/)
{
    auto allReady(false);
    for (; !allReady;) {
        allReady = true;
        for (auto &f : f_vec) {
            auto status = f.wait_for(std::chrono::seconds(interval));
            allReady &= std::future_status::ready == status;
        }
        utils::Timing::outStr("Sent/Received: " + utils::to_freindly_string(client.get_bytes_sent()) + "/" + utils::to_freindly_string(
                                  client.get_bytes_received()));
    }
    return client.get_error();
}

} // namespace utils

TCPClientApp::TCPClientApp()
{
}

TCPClientApp::~TCPClientApp()
{
}

void TCPClientApp::sendDS8File(ConnectionInfo connectionInfo, const std::string &fileName,
                               const std::string &fileNameOut/* = ""*/)
{
    std::cout << __func__ << "(" << fileName << ") started.\n";
    std::ifstream ifs(fileName, std::ios::binary);
    if (!ifs) {
        std::cerr << "File \"" << std::string(fileName) << "\" not found.\n";
        return;
    }
    auto fileSize = utils::convertors::getFileSize(fileName);
    if (fileSize < utils::convertors::ds8binHeader::size()) {
        ifs.close();
        std::cerr << "Wrong size of file.\n";
        return;
    }
    fileSize -= utils::convertors::ds8binHeader::size();
    uint32_t bufSize(1);
    if (!utils::convertors::ds8binHeader::read(ifs, bufSize)) {
        ifs.close();
        std::cerr << "Wrong type of file.\n";
        return;
    }
    if (fileSize % bufSize != 0) {
        ifs.close();
        std::cerr << "Wrong size of file.\n";
        return;
    }
    TCPClient client;
    connectionInfo.tcpBufSize_ = bufSize;
    if (!client.initConnection(connectionInfo)) {
        std::cerr << "Init error\n";
        return;
    }
    if (!client.connect()) {
        return;
    }

    std::vector<char> data(static_cast<size_t>(bufSize));
    auto pData = &data.front();
    auto nBlocks(fileSize / bufSize);
    auto f_rcv = std::async(std::launch::async, receiveToDs8, std::ref(client),
                            fileNameOut.empty() ? fileName + ".out" : fileNameOut);
    int64_t i(0);
    for (; i < nBlocks; i++) {
        ifs.read(pData, bufSize);
        int bytesSent = client.send(pData, static_cast<int>(bufSize));
        if (bytesSent < 0) {
            break;
        }
    }
    f_rcv.get();
}

void TCPClientApp::sendBinFile(ConnectionInfo connectionInfo, const std::string &fileName,
                               const std::string &fileNameOut /*= ""*/)
{
    std::cout << __func__ << "(" << fileName << ") started.\n";
    std::ifstream ifs(fileName, std::ios::binary);
    if (!ifs) {
        std::cerr << "File \"" << std::string(fileName) << "\" not found.\n";
        return;
    }

    TCPClient client;
    if (!client.initConnection(connectionInfo)) {
        std::cerr << "Init error\n";
        return;
    }
    if (!client.connect()) {
        return;
    }

    auto fileSize = size_t(utils::convertors::getFileSize(fileName));

    auto bufSize = static_cast<size_t>(client.get_connection_info().tcpBufSize_);
    auto bufDataSize(bufSize - 16);
    std::vector<utils::DS8WORD> data(static_cast<size_t>(bufSize / 8));
    auto pBlock = data[0].c_8;
    auto pData = data[2].c_8;
    size_t readBytes(0);
    auto dataSize(bufDataSize);
    data[0].w_64 = static_cast<uint64_t>(dataSize);
    data[1].w_64 = DataTypes::data;
    size_t i(0);
    auto needReadBytes(fileSize);

    auto f_rcv = std::async(std::launch::async, receiveToBin, std::ref(client),
                            fileNameOut.empty() ? fileName + ".out" : fileNameOut);

    utils::Timing tm("Sending");
    do {
        if (bufDataSize > needReadBytes) {
            dataSize = needReadBytes;
            memset(pData, 0, bufDataSize);
            //data[0].w_64 = ((dataSize + 7) / 8) * 8;
            data[0].w_64 = dataSize;
        }
        ifs.read(pData, static_cast<int64_t>(dataSize));
        auto bytesRead = ifs.gcount();
        if (bytesRead < 1) {
            break;
        }
        int bytesSent = client.send(pBlock, static_cast<int>(bufSize));
        if (bytesSent < 0) {
            return;
        }
        readBytes += dataSize;
        needReadBytes = fileSize - readBytes;
        ++i;
    } while (needReadBytes > 0);

    int bytes = client.send_terminate();

    tm.outResultStr(client.get_bytes_sent());

    auto rcv_bytes = f_rcv.get();

    std::cout << "Sent/Received " << client.get_bytes_sent() << "/" << client.get_bytes_received() << " bytes" << std::endl;
}

void TCPClientApp::sendHexFile(ConnectionInfo connectionInfo, const std::string &fileName,
                               const std::string &fileNameOut /*= ""*/)
{
    std::cout << __func__ << "(" << fileName << ") started.\n";
    std::ifstream ifs(fileName, std::ios::binary);
    if (!ifs) {
        std::cerr << "File \"" << std::string(fileName) << "\" not found.\n";
        return;
    }

    TCPClient client;
    if (!client.initConnection(connectionInfo)) {
        std::cerr << "Init error\n";
        return;
    }
    if (!client.connect()) {
        return;
    }
    ifs >> std::hex;

    auto bufSize = static_cast<size_t>(client.get_connection_info().tcpBufSize_);
    std::vector<uint64_t> data(static_cast<size_t>(bufSize / 8));
    auto pBlock = reinterpret_cast<char *>(&data[0]);
    auto dataSize(bufSize - 16);
    data[0] = static_cast<uint64_t>(dataSize);
    data[1] = DataTypes::data;
    int bytesSent;

    auto f_rcv = std::async(std::launch::async, receiveToHex, std::ref(client),
                            fileNameOut.empty() ? fileName + ".out" : fileNameOut);

    for (; !ifs.eof();) {
        for (size_t i = 2; i < data.size(); i++) {
            if (!(ifs >> data[i])) {
                data[0] = (i - 2) * sizeof(uint64_t);
                for (auto j = i; j < data.size(); j++) {
                    data[j] = 0;
                }
                break;
            }
        }
        bytesSent = client.send(pBlock, static_cast<int>(bufSize));
        if (bytesSent < 0) {
            return;
        }
    }

    bytesSent = client.send_terminate();
    if (bytesSent < 0) {
        return;
    }
    f_rcv.get();
}

bool TCPClientApp::sendData(TCPClient &client, std::vector<uint64_t> &dataIn)
{
    std::cout << "sendData(size:" << dataIn.size() << ") started.\n";
    size_t dataInSize = dataIn.size() * sizeof(uint64_t);

    auto bufSize = static_cast<size_t>(client.get_connection_info().tcpBufSize_);
    auto bufDataSize(bufSize - 16);
    std::vector<uint64_t> data(static_cast<size_t>(bufSize / 8));
    auto pBlock = reinterpret_cast<char *>(&data[0]);
    auto pData = reinterpret_cast<char *>(&data[2]);
    auto pDataIn = reinterpret_cast<char *>(&dataIn[0]);
    size_t readBytes(0);
    auto dataSize(bufDataSize);
    data[0] = static_cast<uint64_t>(dataSize);
    data[1] = DataTypes::data;

    utils::Timing tm("Sending");

    size_t i(0);
    auto needReadBytes(dataInSize);
    do {
        if (bufDataSize > needReadBytes) {
            dataSize = needReadBytes;
            data[0] = dataSize;
        }
        memcpy(pData, pDataIn + readBytes, dataSize);
        int bytesSent = client.send(pBlock, static_cast<int>(bufSize));
        if (bytesSent < 0) {
            return false;
        }
        readBytes += dataSize;
        needReadBytes = dataInSize - readBytes;
        ++i;
    } while (needReadBytes > 0);

    tm.outResultStr(client.get_bytes_sent());

    int bytes = client.send_terminate();

    return true;
}

int TCPClientApp::send_exit(TCPClient &client)
{
    //if (client.getState() == ConnectionState::Connected && (client.get_bytes_sent() % client.get_connection_info().tcpBufSize_ > 0)) {
    client.disconnect();
    //}
    //if (client.getState() != ConnectionState::Connected) {
    client.connect();
    //}
    auto blockSize = static_cast<int>(client.get_connection_info().tcpBufSize_);
    if (blockSize * client.get_connection_info().nSockets_ == client.send_terminate_exit()) {
        std::cout << "The send of terminate flag and exit flag is executed successfully\n";
        std::vector<char> tmpBuf(client.get_connection_info().tcpBufSize_);
        return client.receive(&tmpBuf[0], blockSize);
    } else {
        std::cerr << "Error of sending the terminate flag and exit flag\n";
    }
    return 0;
}

uint64_t TCPClientApp::receiveToDs8(TCPClient &client, const std::string &fileNameReceive/* = ""*/)
{
    auto bufSize = client.get_connection_info().tcpBufSize_;
    std::ofstream ofs;
    if (!fileNameReceive.empty()) {
        ofs.open(fileNameReceive, std::ofstream::out | std::ios::binary);
        if (!ofs.is_open()) {
            std::cerr << "File \"" << std::string(fileNameReceive) << "\" not created." << std::endl;
            return 0;
        }
        if (!utils::convertors::ds8binHeader::write(ofs, bufSize)) {
            ofs.close();
            return 0;
        }
    }

    std::vector<uint64_t> data(static_cast<size_t>(bufSize / 8));
    auto pDataChar = reinterpret_cast<char *>(&data[0]);
    uint64_t blockCount(0);
    utils::Timing tm("Receiving");
    auto startRcv(true);
    for (;;) {
        int bytes = client.receive(pDataChar, bufSize);
        if (bytes < 0) {
            tm.outStr("Error of receive.");
            break;
        }
        if (startRcv) {
            tm.reStart();
            startRcv = false;
        }
        if (ofs.is_open()/* && data[1] == DataTypes::data*/) {
            ofs.write(pDataChar, bufSize);
        }
        if ((DataTypes::service == data[1] && (ControlTags::terminate == data[2] || ControlTags::terminate == data[3]))) {
            tm.outStr("Terminate tag is received.");
            break;
        }
        ++blockCount;
    }

    tm.outResultStr(blockCount * static_cast<uint64_t>(bufSize));
    //tm.outStr(utils::get_receive_speed_msg(client));
    ofs.close();

    return blockCount;
}

uint64_t TCPClientApp::receiveToHex(TCPClient &client, const std::string &fileNameReceive/* = ""*/)
{
    std::ofstream ofs;
    if (!fileNameReceive.empty()) {
        ofs.open(fileNameReceive, std::ofstream::out | std::ios::binary);
        if (!ofs.is_open()) {
            std::cerr << "File \"" << std::string(fileNameReceive) << "\" not created." << std::endl;
            return 0;
        }
    }

    auto bufSize = client.get_connection_info().tcpBufSize_;
    std::vector<utils::DS8WORD> data(static_cast<size_t>(bufSize / 8));
    auto pChar = data[0].c_8;
    auto exit(false);
    ofs << std::hex;
    size_t rcv_bytes(0);
    do {
        int bytes = client.receive(pChar, bufSize);
        if (bytes < 0) {
            break;
        }
        rcv_bytes += bytes;
        if (ofs.is_open() && DataTypes::data == data[1].w_64) {
            auto data_size = (data[0].w_64 + 7) / 8;
            for (size_t j = 0; j < data_size; j++) {
                ofs << std::setw(16) << std::setfill('0') << data[j + 2].w_64 << "\n";

                if (!ofs.good()) {
                    std::cerr << "Write to file error: " << GetLastError() << "\n";
                    return false;
                }
            }
        }
        if (rcv_bytes % 10000 == 0) {
            std::cout << "\rSent/Received " << client.get_bytes_sent() << "/" << client.get_bytes_received() << " bytes";
        }
        exit = exit || (DataTypes::service == data[1].w_64 && (ControlTags::terminate == data[2].w_64
                                                               || ControlTags::terminate == data[3].w_64));
    } while (!exit);

    ofs.close();
    std::cout << "\rSent/Received " << client.get_bytes_sent() << "/" << client.get_bytes_received() << " bytes" << std::endl;

    return rcv_bytes;
}

uint64_t TCPClientApp::receiveToBin(TCPClient &client, const std::string &fileNameReceive/* = ""*/)
{
    std::ofstream ofs;
    if (!fileNameReceive.empty()) {
        ofs.open(fileNameReceive, std::ofstream::out | std::ios::binary);
        if (!ofs.is_open()) {
            std::cerr << "File \"" << std::string(fileNameReceive) << "\" not created." << std::endl;
            return 0;
        }
    }

    auto bufSize = client.get_connection_info().tcpBufSize_;
    std::vector<utils::DS8WORD> data(static_cast<size_t>(bufSize / 8));
    auto pChar = data[0].c_8;
    auto pDataChar = data[2].c_8;
    auto exit(false);
    size_t rcv_bytes(0);

    utils::Timing tm("Receiving");
    do {
        int bytes = client.receive(pChar, bufSize);
        if (bytes < 0) {
            break;
        }
        rcv_bytes += bytes;
        if (ofs.is_open() && DataTypes::data == data[1].w_64) {
            ofs.write(pDataChar, data[0].w_64);
        }
        exit = exit || (DataTypes::service == data[1].w_64 && (ControlTags::terminate == data[2].w_64
                                                               || ControlTags::terminate == data[3].w_64));
    } while (!exit);

    tm.outResultStr(rcv_bytes);

    ofs.close();

    return rcv_bytes;
}

uint64_t TCPClientApp::receiveToData(TCPClient &client, std::vector<uint64_t> &dataOut)
{
    auto bufSize = client.get_connection_info().tcpBufSize_;
    const auto dataBufSize = bufSize / sizeof(uint64_t);
    if (dataOut.size() < 1) {
        dataOut.resize(dataBufSize * 10);
    }
    auto pDataCharBegin = reinterpret_cast<char *>(&dataOut[0]);
    auto pDataChar(pDataCharBegin);
    auto isTerminate(false);
    auto rcvDataSize(0);
    utils::Timing tm("Receiving");
    do {
        const auto needSize = rcvDataSize + dataBufSize;
        if (dataOut.size() < needSize) {
            dataOut.resize(needSize);
            pDataCharBegin = reinterpret_cast<char *>(&dataOut[0]);
            pDataChar = reinterpret_cast<char *>(&dataOut[rcvDataSize]);
        }
        int bytes = client.receive_data(pDataChar, bufSize, isTerminate);
        if (bytes < 0) {
            return 0;
        }
        if (0 == rcvDataSize) {
            tm.reStart();
        }
        pDataChar += bytes;
        rcvDataSize += (bytes + sizeof(uint64_t) - 1) / sizeof(uint64_t);
    } while (!isTerminate);
    tm.outResultStr(client.get_bytes_received());
    dataOut.resize(rcvDataSize);

    return dataOut.size() * sizeof(uint64_t);
}

int TCPClientApp::Run()
{
    return 0;
}
