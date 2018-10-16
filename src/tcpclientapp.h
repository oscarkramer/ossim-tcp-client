#ifndef TCPCLIENTAPP_H
#define TCPCLIENTAPP_H

#include "tcpclient.h"
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <chrono>
#include <unordered_map>
#include <algorithm>
#include <future>


namespace utils {

union DS8WORD {
    uint64_t w_64;
    uint32_t w_32[2];
    uint16_t w_16[4];
    uint8_t  w_8[8];
    char     c_8[8];
};

class Timing
{
public:
    explicit Timing(const std::string &title);
    ~Timing();

    void outResultStr(const int64_t bytes);
    void outResultStr();
    void reStart();
    static void outStr(const std::string &str);
    static void sleepMs(const int sleepMs);
    long long runTimesNano();

private:
    static std::mutex outMutex_;
    std::string title_;
    std::chrono::high_resolution_clock::time_point t_start_;
    bool is_out_result_;
};

class StdInOutHandler
{
public:
    explicit StdInOutHandler(std::string &inFile, std::string &outFile);
    ~StdInOutHandler();

    void start();
    int getError()
    {
        return error_;
    }

private:
    std::string fileNameIn_;
    std::string fileNameOut_;
    int error_;
    std::ostringstream tmpCoutStream_;
    std::streambuf *prevCoutBuf_;
};

namespace convertors {
enum FileType {
    bin,
    hex,
    ds8,
    mem
};
static std::unordered_map<std::string, FileType> typesOfFile{
    { "bin", FileType::bin },
    { "hex", FileType::hex },
    { "ds8", FileType::ds8 },
    { "mem", FileType::mem },
};

namespace ds8binHeader {
const char DS8_HEADER_TITLE[] = "DS8 binary file";
union DS8_FILE_HEADER {
    struct FIELDS {
        char title[16];
        uint32_t blockSize;
    } fields;
    char buf[16 + sizeof(uint32_t)] = { 0 };
    DS8_FILE_HEADER() {
        //    memset(buf, 0, sizeof(buf));
    }
};
long long size();
bool write(std::ofstream &ofs, const uint32_t blockSize);
bool read(std::ifstream &ifs, uint32_t &blockSize);
} // namespace ds8binHeader

std::ifstream::pos_type getFileSize(const std::string &filename);
bool file_hex_to_ds8(const std::string &fileNameHex, const std::string &fileNameBin, const uint32_t bufSizeB);
bool file_hex_to_bin(const std::string &fileNameHex, const std::string &fileNameBin);
bool file_bin_to_ds8(const std::string &fileName, const std::string &fileNameDs8, const uint32_t bufSizeB);
bool file_bin_to_hex(const std::string &fileName, const std::string &fileNameHex, const bool tag = false);
bool file_ds8_to_bin(const std::string &fileName, const std::string &fileNameBin);
bool file_ds8_to_hex(const std::string &fileName, const std::string &fileNameHex, const bool tag = false);

} //namespace convertors

std::string get_send_speed_msg(TCPClient &client);
std::string get_receive_speed_msg(TCPClient &client);
bool replace_substr(std::string &str, const std::string &from, const std::string &to);
std::string str_to_upper(const std::string &strIn);

template <typename T, unsigned S>
inline unsigned array_size(const T (&v)[S])
{
    return S;
}

template <typename T>
std::string to_string(const T value)
{
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

template <typename T>
std::string num_to_string(const T &val)
{
    struct Numpunct: public std::numpunct<char> {
    protected:
        virtual char do_thousands_sep() const
        {
            return ' ';
        }
        virtual std::string do_grouping() const
        {
            return "\03";
        }
    };
    std::stringstream ss;
    ss.imbue({std::locale(), new Numpunct});
    ss << std::setprecision(3) << std::fixed << val;
    std::string sout(ss.str());
    while (sout.find('.') != std::string::npos && sout.back() == '0' || sout.back() == '.') {
        sout.pop_back();
    }
    return sout;
}

template <typename T>
std::string num_to_hex_string(const T &val)
{
    const auto size_val = sizeof(T);
    std::ostringstream ss;
    ss << std::hex;
    for (int i = 0; i < size_val; i++) {
        ss << std::setfill('0') << std::setw(2) << int(static_cast<short>(val >> ((size_val - i - 1) * 8)) & 0x00FF);
    }
    return ss.str();
}


template <typename T>
std::string to_freindly_string(const T value)
{
    auto str("bit");
    auto ull(static_cast<double>(value * 8));
    auto ll(static_cast<double>(value));

    if (ll > 1) {
        ull = ll;
        ll /= 1024;
        str = "B";
        if (ll > 1) {
            ull = ll;
            ll /= 1024;
            str = "KB";
            if (ll > 1) {
                ull = ll;
                ll /= 1024;
                str = "MB";
                if (ll > 1) {
                    ull = ll;
                    ll /= 1024;
                    str = "GB";
                    if (ll > 1) {
                        ull = ll;
                        ll /= 1024;
                        str = "TB";
                    }
                }
            }
        }
    }
    std::string sout(num_to_string(ull));
    sout += " ";
    sout += str;
    return sout;
}

int status_out(std::vector<std::future<uint64_t>> &f_vec, TCPClient &client, const long long interval = 5);

} // namespace utils

class TCPClientApp
{
public:
    TCPClientApp();
    virtual ~TCPClientApp();
    virtual int Run();

    static void sendDS8File(ConnectionInfo connectionInfo, const std::string &fileName, const std::string &fileNameOut = "");
    static void sendBinFile(ConnectionInfo connectionInfo, const std::string &fileName, const std::string &fileNameOut = "");
    static void sendHexFile(ConnectionInfo connectionInfo, const std::string &fileName, const std::string &fileNameOut = "");
    static bool sendData(TCPClient &client, std::vector<uint64_t> &dataIn);
    static int send_exit(TCPClient &client);
    static uint64_t receiveToDs8(TCPClient &client, const std::string &fileNameReceive = "");
    static uint64_t receiveToBin(TCPClient &client, const std::string &fileNameReceive = "");
    static uint64_t receiveToHex(TCPClient &client, const std::string &fileNameReceive = "");
    static uint64_t receiveToData(TCPClient &client, std::vector<uint64_t> &dataOut);

private:
    TCPClient client_;
};

#endif // TCPCLIENTAPP_H
