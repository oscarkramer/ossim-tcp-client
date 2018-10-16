#include "tcpclient.h"

#include <iomanip>
#include <iostream>
#include <vector>
#include <sstream>
#include <algorithm>


//#define _DEBUG_INFO
#ifdef _DEBUG_INFO
#define DISPLAY_INFO(str) std::cout << std::string(str) << "\n";
#else
#define DISPLAY_INFO(str)
#endif

namespace Version {
std::string to_string(const uint64_t &ui64)
{
    ui64_ui8 ver;
    ver.ui64 = ui64;
    std::string version_str("");
    for (auto i = 3; i >= 0; --i) {
        version_str += std::to_string(ver.ui8[i]);
        if (i > 0) {
            version_str += ".";
        }
    }
    return version_str;
}
bool TcpIoBlock::check(const uint64_t &ui64)
{
    auto res = ui64 >= gMinVersion;
    if (!res) {
        std::cerr << "Wrong version of tcp_io_block: " << to_string(ui64) << "\n"
                  << "tcp_io_block vesion must be >= " << to_string(gMinVersion)
                  << std::endl;
    }

    return res;
}
} // namespace Version

namespace {
void uInt64ToChar8(uint64_t i64, void *char8buf)
{
    memcpy(char8buf, &i64, sizeof(i64));
}
void char8ToUint64(void *char8buf, uint64_t &i64)
{
    memcpy(&i64, char8buf, sizeof(i64));
}
}  // namespace

TCPClient::TCPClient() : bytesSent_(0), bytesReceived_(0), sentTime_(0), receivedTime_(0), lastError_(0)
{
    //const uint64_t gVersion = 0x01000001;
    std::cout << "TCP client library version: " << Version::to_string(Version::TcpClientLibrary::gVersion) << std::endl;
}

TCPClient::~TCPClient()
{
#ifdef _WIN32
    if (connection_.state_ == ConnectionState::Initialized) {
        WSACleanup();
    }
#endif
}

std::mutex TCPClient::mutex_;


void TCPClient::SleepMs(int sleepMs)
{
#ifdef _WIN32
    Sleep(static_cast<uint16_t>(sleepMs));
#else
    usleep(sleepMs * 1000);
#endif
}

bool TCPClient::initConnection(const ConnectionInfo &info)
{
    bool result(false);
#ifdef _WIN32
    WSADATA wsaData;
    // Initialize Winsock
    int iResult = WSAStartup(static_cast<uint16_t>(MAKEWORD(2, 2)), &wsaData);
    if (iResult != 0) {
        std::cerr << "WSAStartup failed with error: " << iResult << std::endl;
        set_error("Initialization of tcpclient is failed");
        return false;
    }
#endif
    DISPLAY_INFO("Initialization of connection to " + info.remoteAddress_);
    const auto host(getHostByName(info.remoteAddress_));
    const auto host_address(inet_addr(host.c_str()));
    connection_.server_addr_.sin_family = AF_INET;
    connection_.server_addr_.sin_port = htons(info.port_);
    connection_.server_addr_.sin_addr.s_addr = host_address;
    //connection_.sockfd_.push_back(0);

    if (connectionInfo_ != info) {
        connectionInfo_ = info;
        connection_.state_ = ConnectionState::Initialized;
        result = true;
    }
    return result;
}

int getSockOpt(const DS_SOCKET socket, const int optname)
{
    auto optval(0);
    socklen_t optlen(sizeof(optval));
    if (getsockopt(socket, SOL_SOCKET, optname, (char *)&optval, &optlen) != 0) {
        std::cerr << errno << ": Error get socket option." << std::endl;
    }
    return optval;
}

int setSockOpt(const DS_SOCKET socket, const int optname, const int optval)
{
    socklen_t optlen(sizeof(optval));
    if (setsockopt(socket, SOL_SOCKET, optname, (char *)&optval, optlen) != 0) {
        std::cerr << errno << ": Error set socket option." << std::endl;
    }
    return getSockOpt(socket, optname);
}

bool TCPClient::configSocket(const size_t &idx_socket, const DS_SOCKET &sock, size_t &socketID)
{
    std::vector<uint64_t> buf(8);
    const int ctrl_buf_size = int(buf.size() * sizeof(uint64_t));
    char *pBuf = reinterpret_cast<char *>(&buf[0]);
    int sec_left(connectionInfo_.waitConnect_);

    // Receive of control block with number of socket
    setTimeout(sock, true, wait_step_sec);
    int rBytes(0);
    do {
        rBytes = ::recv(sock, pBuf, ctrl_buf_size, MSG_WAITALL);
        if (rBytes < ctrl_buf_size) {
            if (sec_left >= wait_step_sec) {
                sec_left -= wait_step_sec;
                std::cout << "Waiting of control block..." << sec_left << "s\n";
            } else {
                set_error("Error connecting");
                return false;
            }
        }
    } while (rBytes < 0);
    if (connectionInfo_.displayRaw_) {
        display_data(pBuf, ctrl_buf_size);
    }

    bool res(false);
    int buf_size_send = getSockOpt(sock, SO_SNDBUF);
    int buf_size_receive = getSockOpt(sock, SO_RCVBUF);
    if (DataTypes::service == buf[1] && ControlTags::socketconfig == buf[2] && ctrl_buf_size == buf[0] + sizeof(uint64_t)) {
        socketID = buf[4];
        if (0 == idx_socket) {
            auto version = buf[3];
            if (!Version::TcpIoBlock::check(version)) {
                return false;
            }
            connectionInfo_.tcpBufSize_ = uint32_t(buf[5]);
            connectionInfo_.nSockets_ = uint8_t(buf[6]);
            connectionInfo_.isDuplexSockets_ = 1 == buf[7];
            std::string title("*********** TCP client parameters: ***********");
            std::cout << title << "\n"
                      << "tcp_io_block version:                  " << Version::to_string(version) << "\n"
                      << "Host:                                  " << connectionInfo_.remoteAddress_ << "\n"
                      << "Port:                                  " << connectionInfo_.port_ << "\n"
                      << "Count of sockets:                      " << std::to_string(connectionInfo_.nSockets_) << "\n"
                      << "Use duplex mode of socket:             " << std::string(connectionInfo_.isDuplexSockets_ ? "yes" : "no") << "\n"
                      << "TCP block size in bytes:               " << connectionInfo_.tcpBufSize_ << "\n"
                      << "Delay after every receiving of data:   " << connectionInfo_.delayRcvMs_ << " ms\n"
                      << "Delay after every sending of data:     " << connectionInfo_.delaySendMs_ << " ms\n"
                      << "Timeout:                               " << connectionInfo_.timeOut_ << " sec\n"
                      << "Print info to console:                 " << (connectionInfo_.displayRaw_ ? "yes" : "no") << "\n"
                      << "System socket send buffer size:        " << buf_size_send << "\n"
                      << "System socket receive buffer size:     " << buf_size_receive << "\n"
                      << std::string(title.length(), '*') << std::endl;
            connection_.sockfd_.resize(connectionInfo_.nSockets_, 0);
            if (connectionInfo_.isDuplexSockets_) {
                connection_.sockfd_send_.resize(connectionInfo_.nSockets_, 0);
                connection_.sockfd_rcv_.resize(connectionInfo_.nSockets_, 0);
            } else {
                connection_.sockfd_send_.resize(connectionInfo_.nSockets_ / 2, 0);
                connection_.sockfd_rcv_.resize(connectionInfo_.nSockets_ / 2, 0);
            }
        }
        res = true;
    }
    if (res) {
        const int new_buf_size = connectionInfo_.tcpBufSize_;
        // set receive and send TCP buffer size
        if (new_buf_size > buf_size_send) {
            buf_size_send = setSockOpt(sock, SO_SNDBUF, new_buf_size);
        }
        if (new_buf_size > buf_size_receive) {
            buf_size_receive = setSockOpt(sock, SO_RCVBUF, new_buf_size);
        }

        buf.resize(size_t(std::ceil(new_buf_size / 8)));
        pBuf = reinterpret_cast<char *>(&buf[0]);

        rBytes = ::recv(sock, pBuf, new_buf_size - ctrl_buf_size, MSG_WAITALL);
        res = rBytes == new_buf_size - ctrl_buf_size;
        if (res) {
            std::cout << "Socket number:                     " << (idx_socket + 1) << "\n"
                      << "Socket ID:                         " << sock << "\n"
                      << "Client socket send buffer size:    " << buf_size_send << "\n"
                      << "Client socket receive buffer size: " << buf_size_receive << std::endl;

            connection_.sockfd_[socketID] = sock;

            if (connectionInfo_.isDuplexSockets_) {
                connection_.sockfd_send_[socketID] = sock;
                connection_.sockfd_rcv_[socketID] = sock;
                std::cout << "Type of using socket:               " << "Out&In" << std::endl;
            } else {
                size_t div_2 = connectionInfo_.nSockets_ / 2;
                if (socketID < div_2) {
                    connection_.sockfd_send_[socketID] = sock;
                    std::cout << "Type of using socket:              " << "Out" << std::endl;
                } else {
                    connection_.sockfd_rcv_[socketID - div_2] = sock;
                    std::cout << "Type of using socket:              " << "In" << std::endl;
                }
            }
            std::cout << std::string(40, '*') << std::endl;
        }
    }
    if (!res) {
        set_error("Error connecting");
    }
    return res;
}

bool TCPClient::connect()
{
    auto count_sockets(0);
    do {
        //auto &sock = connection_.sockfd_[idx];
        auto sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        // create a connection with the server
        int err_connect(0);
        int sec_left(connectionInfo_.waitConnect_);
        std::cout << "Connecting of TCP socket " << (count_sockets + 1) << "...";
        do {
            err_connect = ::connect(sock, reinterpret_cast<struct sockaddr *>(&connection_.server_addr_),
                                    sizeof(connection_.server_addr_));
            if (err_connect < 0) {
                if (sec_left >= wait_step_sec) {
                    SleepMs(wait_step_sec * 1000);
                    sec_left -= wait_step_sec;
                    std::cout << "Waiting of connect..." << sec_left << "s\n";
                } else {
                    set_error("Error connecting");
                    return false;
                }
            }
        } while (err_connect < 0);
        std::cout << "CONNECTED" << std::endl;

        size_t socketID;
        if (!configSocket(count_sockets, sock, socketID)) {
            return false;
        }

        if (connectionInfo_.delayAfterConnect_ > 0) {
            std::cout << "Sleep " << connectionInfo_.delayAfterConnect_ << " ms" << std::endl;
            SleepMs(connectionInfo_.delayAfterConnect_);
        }
    } while (++count_sockets < connection_.sockfd_.size());

    if (connectionInfo_.timeOut_ >= 0) {
        setTimeout(connectionInfo_.timeOut_);
    }
    connection_.state_ = ConnectionState::Connected;
    connection_.iSocketSend_ = 0;
    connection_.iSocketRcv_ = 0;
    bytesSent_ = 0;
    bytesReceived_ = 0;

    return true;
}

bool TCPClient::disconnect()
{
    for (auto &sock : connection_.sockfd_) {

#ifdef _WIN32
        shutdown(sock, SD_BOTH);
        closesocket(sock);
#else
        close(sock);
#endif
        sock = 0;
    }
    connection_.state_ = ConnectionState::Disconnected;
    return true;
}

ConnectionState TCPClient::getState() const
{
    return connection_.state_;
}

int TCPClient::send_terminate()
{
    if (get_connection_info().exit_) {
        return send_terminate_exit();
    }
    std::vector<uint64_t> data(connectionInfo_.tcpBufSize_ / 8, 0);
    data[0] = 8;
    data[1] = DataTypes::service;
    data[2] = ControlTags::terminate;
    int bytes(0);
    for (auto sock : connection_.sockfd_send_) {
        bytes += send(reinterpret_cast<char *>(&data[0]), int(data.size()) * 8);
    }
    if (connection_.sockfd_send_.size() * connectionInfo_.tcpBufSize_ == bytes) {
        std::cout << "The terminate command is sent." << std::endl;
    }
    return bytes;
}

int TCPClient::send_terminate_exit()
{
    std::vector<uint64_t> data(connectionInfo_.tcpBufSize_ / 8, 0);
    data[0] = 2 * 8;
    data[1] = DataTypes::service;
    data[2] = ControlTags::exit;
    data[3] = ControlTags::terminate;
    int bytes(0);
    for (auto sock : connection_.sockfd_send_) {
        bytes += send(reinterpret_cast<char *>(&data[0]), int(data.size()) * 8);
    }
    if (connection_.sockfd_send_.size() * connectionInfo_.tcpBufSize_ == bytes) {
        std::cout << "The terminate and exit commands is sent." << std::endl;
    }
    return bytes;
}

int TCPClient::send_to_tcp_io(const char *dataIn, const int length)
{
    if (dataIn == nullptr) {
        return 0;
    }
    auto bufSize = get_connection_info().tcpBufSize_;
    auto bufDataSize(bufSize - 16);
    std::vector<uint64_t> data(static_cast<size_t>(bufSize / 8));
    auto pBlock = reinterpret_cast<char *>(&data[0]);
    auto pData = reinterpret_cast<char *>(&data[2]);
    size_t readBytes(0);
    auto dataSize(bufDataSize);
    data[0] = static_cast<uint64_t>(dataSize);
    data[1] = DataTypes::data;
    auto needReadBytes(length);
    do {
        if (bufDataSize > static_cast<uint32_t>(needReadBytes)) {
            dataSize = needReadBytes;
            data[0] = dataSize;
        }
        memcpy(pData, dataIn + readBytes, dataSize);
        int bytesSent = send(pBlock, static_cast<int>(bufSize));
        if (bytesSent < 0) {
            return -1;
        }
        readBytes += dataSize;
        needReadBytes = length - int(readBytes);
    } while (needReadBytes > 0);

    return length;
}

int TCPClient::send_sync(const std::vector<char> &data)
{
    int length(static_cast<int>(data.size()));
    if (0 == length) {
        return 0;
    }
    return send_to_tcp_io(&data.front(), length);
}

int TCPClient::send(const char *data, const int length)
{
    if (get_error() != 0) {
        return -2;
    }
    if (data == nullptr) {
        return 0;
    }

    auto idxSocket(connection_.iSocketSend_++);
    if (connection_.iSocketSend_ == connection_.sockfd_send_.size()) {
        connection_.iSocketSend_ = 0;
    }
    auto bytesSent(0);
    while (bytesSent < length) {
        auto t1 = std::chrono::high_resolution_clock::now();
        auto bytes = ::send(connection_.sockfd_send_[idxSocket], data + bytesSent, length - bytesSent, 0);
        sentTime_ += std::chrono::duration<uint64_t, std::nano>(std::chrono::high_resolution_clock::now() - t1).count();
        //auto t2 = std::chrono::high_resolution_clock::now();
        //sentTime_ += t2 - t1;

        if (bytes < 0) {
            set_error("Error write to socket");
            return -1;
        } else {
            bytesSent += bytes;
        }
    }
    bytesSent_ += bytesSent;
    if (connectionInfo_.displayRaw_) {
        display_data(data, bytesSent, "Send buffer to socket #" +
                     std::to_string(idxSocket + 1) +
                     " ID: " + std::to_string(connection_.sockfd_send_[idxSocket]));
    }
    SleepMs(connectionInfo_.delaySendMs_);
    return bytesSent;
}

int TCPClient::send_need(char *data, int length)
{
    if (data == nullptr) {
        return 0;
    }
    auto stayLength(length);
    for (;;) {
        auto bytesSent = send(data + (length - stayLength), stayLength);
        if (bytesSent < 0) {
            auto err = GetLastError();
            std::cerr << err << " - Error write to socket\n";
            return bytesSent;
        }
        stayLength -= bytesSent;
        if (stayLength == 0) {
            break;
        }
    }
#ifdef _DEBUG_INFO
    display_data(data, static_cast<size_t>(length));
#endif
    return length;
}

//int TCPClient::send_padding()
//{
//    auto sent_bytes(0);
//    auto byteModBuffSize = (bytesSent_ + tcpSendVec_.size()) % connectionInfo_.tcpBufSize_;
//    if (byteModBuffSize > 0) {
//        std::vector<char> tcp_buffer(uint16_t(connectionInfo_.tcpBufSize_ - byteModBuffSize), '\0');
//        sent_bytes = send_need(&tcp_buffer.front(), int(tcp_buffer.size()));
//    }
//    if (connectionInfo_.displayRaw_) {
//        std::cout << "All bytes sent (" << bytesSent_ << ").\n";
//    }
//    return sent_bytes;
//}

void TCPClient::set_display(const bool displayRaw)
{
    connectionInfo_.displayRaw_ = displayRaw;
}

//void TCPClient::set_tcp_buf_size(const int buf_size)
//{
//    connectionInfo_.tcpBufSize_ = buf_size;
//    tcpRcvVec_.reserve(connectionInfo_.tcpBufSize_ * 1024);
//    tcpSendVec_.reserve(connectionInfo_.tcpBufSize_ * 1024);
//    tcpTmpBuf_.resize(connectionInfo_.tcpBufSize_, '\0');
//}

int TCPClient::receive_from_socket(const DS_SOCKET &socket, char *data, const int length)
{
    int bytes = ::recv(socket, data, length, MSG_WAITALL);
    if (bytes < length) {
        set_error("Error read from socket");
        return -1;
    } else {
        bytesReceived_ += bytes;
        if (connectionInfo_.displayRaw_) {
            display_data(data, bytes, "Received buffer from socket #" + std::to_string(connection_.iSocketRcv_) + " ID: " + std::to_string(
                             socket));
        }
    }
    return bytes;
}

int TCPClient::receive_data(char *data, int length, bool &is_terminate)
{
    if (get_error() != 0) {
        return -2;
    }
    if (data == nullptr || length < static_cast<int>(connectionInfo_.tcpBufSize_)) {
        std::cerr << "TCPClient::receive: Wrong input parameters." << std::endl;
        return -1;
    }
    auto idxSocketRcv(connection_.iSocketRcv_++);
    if (connection_.iSocketRcv_ == connection_.sockfd_rcv_.size()) {
        connection_.iSocketRcv_ = 0;
    }
    DS_SOCKET &sock = connection_.sockfd_rcv_[idxSocketRcv];

    std::vector<uint64_t> tmpBuf(connectionInfo_.tcpBufSize_ / 8, 0);
    char *pTmpBuf = reinterpret_cast<char *>(&tmpBuf[0]);
    int bytesOfData(0);
    int bytes(0);

    is_terminate = false;

    for (;;) {
        // receiving header
        bytes = receive_from_socket(sock, pTmpBuf, 16);
        if (get_error() != 0) {
            break;
        }
        // receiving data
        if (DataTypes::data == tmpBuf[1]) {
            bytes = receive_from_socket(sock, data, static_cast<int>(tmpBuf[0]));
            if (get_error() != 0) {
                break;
            }
            bytesOfData += bytes;
            // receiving of align (garbage)
            bytes = receive_from_socket(sock, pTmpBuf + 16, static_cast<int>(connectionInfo_.tcpBufSize_ - 16 - bytes));
            if (get_error() != 0) {
                break;
            }
        } else {
            bytes = receive_from_socket(sock, pTmpBuf + 16, static_cast<int>(connectionInfo_.tcpBufSize_ - 16));
            if (get_error() != 0) {
                break;
            }
            is_terminate = DataTypes::service == tmpBuf[1] && (ControlTags::terminate == tmpBuf[2] || ControlTags::terminate == tmpBuf[3]);
        }
        break;
    }

    if (get_error() != 0) {
        //set_error("Error read from socket");
        return -1;
    }
    SleepMs(connectionInfo_.delayRcvMs_);

    return bytesOfData;
}

int TCPClient::receive(char *data, int length)
{
    if (get_error() != 0) {
        return -2;
    }
    if (data == nullptr || length < static_cast<int>(connectionInfo_.tcpBufSize_)) {
        std::cerr << "TCPClient::receive: Wrong input parameters." << std::endl;
        return 0;
    }
    auto idxSocketRcv(connection_.iSocketRcv_++);
    if (connection_.iSocketRcv_ == connection_.sockfd_rcv_.size()) {
        connection_.iSocketRcv_ = 0;
    }
    uint32_t bytesRcv(0);
    while (bytesRcv < connectionInfo_.tcpBufSize_) {
        auto t1 = std::chrono::high_resolution_clock::now();
        auto bytes = ::recv(connection_.sockfd_rcv_[idxSocketRcv], data + bytesRcv,
                            static_cast<int>(connectionInfo_.tcpBufSize_ - bytesRcv), MSG_WAITALL);
        //auto t2 = std::chrono::high_resolution_clock::now();
        if (bytesReceived_ > 0) {
            //receivedTime_ += t2 - t1;
            receivedTime_ += std::chrono::duration<uint64_t, std::nano>(std::chrono::high_resolution_clock::now() - t1).count();
        }

        if (bytes < 0) {
            set_error("Error read from socket");
            return bytes;
        } else {
            bytesRcv += static_cast<uint32_t>(bytes);
        }
    }
    bytesReceived_ += bytesRcv;
    if (connectionInfo_.displayRaw_) {
        display_data(data, static_cast<size_t>(length), "Received buffer from socket #" +
                     std::to_string(idxSocketRcv + 1) +
                     " ID: " + std::to_string(connection_.sockfd_rcv_[idxSocketRcv]));
    }
    SleepMs(connectionInfo_.delayRcvMs_);
    return static_cast<int>(bytesRcv);
}

int TCPClient::receive_need(char *data, int length)
{
    if (data == nullptr) {
        return 0;
    }
    auto stayLength(length);
    for (;;) {
        int bytesReceived = receive(data + (length - stayLength), stayLength);
        if (bytesReceived < 0) {
            auto err = GetLastError();
            std::cerr << err << " - Error read from socket" << std::endl;
            return bytesReceived;
        }
        stayLength -= bytesReceived;
        if (stayLength == 0) {
            break;
        }
    }

    return length;
}

int TCPClient::receive_tcp_io(char *pdata, int length)
{
    if (pdata == nullptr) {
        return 0;
    }

    auto bufSize = get_connection_info().tcpBufSize_;
    std::vector<uint64_t> dataBuf(static_cast<size_t>(bufSize / 8));
    auto pBufChar = reinterpret_cast<char *>(&dataBuf[0]);
    auto pDataChar = reinterpret_cast<char *>(&dataBuf[2]);
    uint64_t readBytes(0);
    bool exit(false);
    do {
        int bytes = receive(pBufChar, static_cast<int>(bufSize));
        if (bytes < 0) {
            return bytes;
        }
        if (static_cast<int>(readBytes) + bytes > length) {
            return -2;
        }
        if (DataTypes::data == dataBuf[1]) {
            memcpy(pdata + readBytes, pDataChar, static_cast<size_t>(dataBuf[0]));
            readBytes += dataBuf[0];
        }
        exit = exit || (DataTypes::service == dataBuf[1] && (ControlTags::terminate == dataBuf[2]
                                                             || ControlTags::terminate == dataBuf[3]));
    } while (!exit);

    return static_cast<int>(readBytes);
}

int TCPClient::receive_sync(std::vector<char> &data)
{
    size_t allBytesReceived(0);
    char header[8 * 2];

    auto readBytes = receive_need(header, sizeof(header));
    if (readBytes < 0) {
        return 0;
    }

    if (sizeof(header) == readBytes) {
        uint64_t dataSize;
        char8ToUint64(&header[0], dataSize);
        auto dataSizeInPad = static_cast<size_t>(((dataSize + 7) / 8) * 8);
        data.reserve(dataSizeInPad);
        uint64_t controlFrame;
        char8ToUint64(&header[8], controlFrame);

        if (DataTypes::service == controlFrame) {
            DISPLAY_INFO("Service tag received.");
            uint64_t term;
            readBytes = receive_need(reinterpret_cast<char *>(&term), sizeof(term));
            if (readBytes < 0) {
                return -1;
            }
            if (term == ControlTags::terminate) {
                DISPLAY_INFO("Termination received.");
            }
        } else if (DataTypes::data == controlFrame) {
            auto bytesReceived =
                receive_need(&data.front() + allBytesReceived, static_cast<int>(dataSizeInPad));
            if (bytesReceived < 0) {
                return -1;
            }
            allBytesReceived += static_cast<size_t>(bytesReceived);
            if (static_cast<uint64_t>(allBytesReceived) >= dataSize) {  // all data is read
                DISPLAY_INFO("All data is read.");
                allBytesReceived = static_cast<size_t>(dataSize);
                data.resize(allBytesReceived);
            }
        } else {
            DISPLAY_INFO("Unsupported cotrol tag.");
            return -1;
        }
    }

    return static_cast<int>(allBytesReceived);
}

std::string TCPClient::getHostByName(const std::string &host) const
{
    struct addrinfo *result(nullptr);
    struct addrinfo addr_info;
    memset(&addr_info, '\0', sizeof(addr_info));
    addr_info.ai_family = AF_INET;
    addr_info.ai_socktype = SOCK_STREAM;
    addr_info.ai_protocol = IPPROTO_TCP;

    if (0 != ::getaddrinfo(host.c_str(), nullptr, &addr_info, &result)) {
        return "";
    }
    const std::string ip_name(inet_ntoa((reinterpret_cast<struct sockaddr_in *>
                                         (result->ai_addr))->sin_addr));
    DISPLAY_INFO("Host IP address: " + ip_name);
    freeaddrinfo(result);

    return ip_name;
}

void TCPClient::setTimeout(const DS_SOCKET sock, const bool is_receive, long to)
{
#ifdef WIN32
    int timeout = to * 1000;
#else
    timeval timeout = { 0, 0 };
    timeout.tv_sec = to;
#endif
    const int optname = is_receive ? SO_RCVTIMEO : SO_SNDTIMEO;
    //std::cout << "before: " << getSockOpt(sock, optname) << std::endl;
    if (setsockopt(sock, SOL_SOCKET, optname, reinterpret_cast<const char *>(&timeout),
                   sizeof(timeout)) < 0) {
        std::cerr << "setsockopt failed\n";
    }
    //std::cout << "after: " << getSockOpt(sock, optname) << std::endl;
    //std::cout << "Set " << std::string(is_receive ? "receive" : "send") << " timeout to: " << getSockOpt(sock, optname) << std::endl;
}

void TCPClient::setTimeout(long to)
{
    for (auto &sock : connection_.sockfd_rcv_) {
        setTimeout(sock, true, to);
    }
    for (auto &sock : connection_.sockfd_send_) {
        setTimeout(sock, false, to);
    }
}

void TCPClient::display_data(const char *data, size_t len, const std::string &strTitle)
{
    std::stringstream ss;
    auto bytesCounter = 0;
    ss << "\n" << strTitle << " (" << len << "):\n";
    ss << std::string(16, '-') << "\n";
    for (size_t i = 0; i < len; ++i) {
        ss << /*"0x" <<*/ std::hex << std::setfill('0') << std::setw(2)
           << (static_cast<short>(data[i]) & 0x00FF);
        bytesCounter++;
        if (bytesCounter >= 8) {
            bytesCounter = 0;
            ss << "\n";
        }
    }
    ss << std::dec;
    if (len % 8 > 0) {
        ss << "\n";
    }
    ss << std::string(16, '-') << "\n";

    static std::mutex mtx;
    mtx.lock();
    std::cout << ss.str() << std::flush;
    mtx.unlock();
}

void TCPClient::set_error(const std::string &msg)
{
    lastError_ = GetLastError();
    if (lastError_ == 0) {
        out_str("ERROR: " + msg + ".", std::cerr);
        lastError_ = -1;
    } else {
        out_str("ERROR: " + std::to_string(lastError_) + " " + msg + ".", std::cerr);
    }
    disconnect();
}

int TCPClient::get_error()
{
    return lastError_;
}

void TCPClient::out_str(const std::string &str, std::ostream &stream)
{
    //std::cout << str << std::endl;
    static std::mutex outMutex;
    outMutex.lock();
    stream << str << std::endl;
    outMutex.unlock();
}

ReceiveChunks::ReceiveChunks(TCPClient *client)
    : client_(client), dataSize_(0), status_(StatusInfo::UnsupportTag)
{
    receive_header();
}

ReceiveChunks::~ReceiveChunks() {}

StatusInfo ReceiveChunks::receive_header()
{
    char header[8 * 2];
    auto readBytes = client_->receive_need(header, sizeof(header));
    if (sizeof(header) != readBytes) {
        status_ = StatusInfo::Error;
    } else {
        char8ToUint64(&header[0], dataSize_);
        needBytesRecieve_ = dataSize_;
        uint64_t conrolFrame;
        char8ToUint64(&header[8], conrolFrame);
        if (DataTypes::service == conrolFrame) {
            DISPLAY_INFO("Service tag received.");
            uint64_t term;
            readBytes = client_->receive_need((char *)&term, sizeof(term));
            if (readBytes < 0) {
                status_ = StatusInfo::Error;
            } else if (ControlTags::terminate == term) {
                DISPLAY_INFO("Termination received.");
                if (StatusInfo::AllData == status_) {
                    status_ = StatusInfo::Finish;
                } else {
                    status_ = StatusInfo::Termination;

                }
            }
        } else if (DataTypes::data == conrolFrame) {
            status_ = StatusInfo::DataProcessing;
            needBytesRecieve_ = dataSize_;
        } else if (DataTypes::padding == conrolFrame) {
            status_ = StatusInfo::DataPadding;
            std::vector<char> tmpBuf(static_cast<size_t>(dataSize_));
            readBytes = client_->receive_need(&tmpBuf.front(), static_cast<int>(dataSize_));
            if (readBytes == dataSize_) {
                status_ = StatusInfo::AllData;
            } else {
                status_ = StatusInfo::Error;
            }
        } else {
            DISPLAY_INFO("Unsupported cotrol tag.");
            status_ = StatusInfo::UnsupportTag;
        }
    }
    return status_;
}

StatusInfo ReceiveChunks::getStatus()
{
    return status_;
}

uint64_t ReceiveChunks::getDataSize()
{
    return dataSize_;
}

int ReceiveChunks::receive_data_block(char *data, int length)
{
    if (status_ != StatusInfo::DataProcessing) {
        return 0;
    }
    if (needBytesRecieve_ < length) {
        length = static_cast<int>(needBytesRecieve_);
    }
    auto bytesReceived = client_->receive_need(data, length);
    if (bytesReceived < 0) {
        status_ = StatusInfo::Error;
        return -1;
    } else {
        needBytesRecieve_ -= bytesReceived;
        if (needBytesRecieve_ <= 0) {
            DISPLAY_INFO("All data is read.");
            status_ = StatusInfo::AllData;
            //client_->receive_padding();
            size_t padding = (dataSize_ + 16) % client_->get_connection_info().tcpBufSize_;
            if (padding > 0) {
                padding = client_->get_connection_info().tcpBufSize_ - padding;
                std::vector<char> dataPad(padding);
                client_->receive_need(&dataPad.front(), int(padding));
            }
            /// We read bytes which were added for alignment to 8
            size_t bytesForPad8 = dataSize_ % 8;
            if (bytesForPad8 > 0) {
                std::vector<char> dataPad(8 - bytesForPad8);
                auto bytesRead = client_->receive_need(&dataPad.front(),
                                                       static_cast<int>(dataPad.size()));
                if (bytesRead != dataPad.size()) {
                    status_ = StatusInfo::Error;
                    return bytesReceived;
                }
            }
            receive_header();
        }
    }
    return bytesReceived;
}

SendChunks::SendChunks(TCPClient *client, const uint64_t dataSize, const uint64_t blockSize)
    : client_(client)
    , dataSize_(dataSize)
    , needBytesSend_(dataSize)
    , status_(StatusInfo::DataProcessing)
    , blockSize_(blockSize)
{
}

SendChunks::~SendChunks() {}

StatusInfo SendChunks::getStatus()
{
    return status_;
}

uint64_t SendChunks::getDataSize()
{
    return dataSize_;
}

int SendChunks::send_header()
{
    std::vector<char> header(16);
    uInt64ToChar8(dataSize_, &header.front());
    uInt64ToChar8(DataTypes::data, &header[8]);
    auto sent_bytes = client_->send_need((char *)&header.front(),
                                         static_cast<int>(header.size()));
    if (header.size() == sent_bytes) {
        status_ = StatusInfo::DataProcessing;
    } else {
        status_ = StatusInfo::Error;
    }
    return sent_bytes;
}

int SendChunks::send_data_block(char *data, int length)
{
    if (needBytesSend_ == dataSize_) {
        /*auto bytesHeader = */send_header();
    }
    if (status_ != StatusInfo::DataProcessing) {
        return 0;
    }
    if (needBytesSend_ < static_cast<uint64_t>(length)) {
        length = static_cast<int>(needBytesSend_);
    }
    auto bytesSent = client_->send_need(data, length);
    if (bytesSent < 0) {
        status_ = StatusInfo::Error;
        return -1;
    }
    needBytesSend_ -= static_cast<uint64_t>(bytesSent);
    if (needBytesSend_ <= 0) {
        DISPLAY_INFO("All data is sent.");
        status_ = StatusInfo::AllData;
        size_t bytesForPad8 = 8 - (dataSize_ % 8);
        if (bytesForPad8 < 8) {
            std::vector<char> dataPad(bytesForPad8, '\0');
            bytesSent += client_->send_need(&dataPad.front(), static_cast<int>(dataPad.size()));
        }
    }
    return bytesSent;
}

SendBlocks::SendBlocks(TCPClient *client, const uint64_t dataSize,
                       const unsigned int blockSize)
    : SendChunks(client, dataSize)
    , blockSize_(blockSize)
    , bytesCount_(0)
{
}

int SendBlocks::send_header()
{
    auto bytesSent(0);
    //    auto bytesBlockSize = client_->send_block_size(blockSize_);
    //    if (bytesBlockSize < 0) {
    //        status_ = StatusInfo::Error;
    //        return bytesBlockSize;
    //    }
    //    bytesSent += bytesBlockSize;
    std::vector<char> header(16);
    uInt64ToChar8(dataSize_, &header.front());
    uInt64ToChar8(DataTypes::data, &header[8]);
    auto bytesHeader = client_->send_need(&header.front(), int(header.size()));

    if (header.size() == bytesHeader) {
        status_ = StatusInfo::DataProcessing;
        bytesSent += bytesHeader;
    } else {
        status_ = StatusInfo::Error;
    }

    return bytesSent;
}
