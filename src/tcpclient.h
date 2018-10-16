/** @file tcpclient.h
 * @brief Header file for using TCP client library
 */
#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <string>
#include <vector>
#include <mutex>
#include <chrono>

#ifdef _WIN32

#if !defined(_WINSOCK_DEPRECATED_NO_WARNINGS)
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#define D_EWOULDBLOCK 10035

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdint.h>

typedef SOCKET DS_SOCKET;

#else // Unix

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#define GetLastError() errno
#define D_EWOULDBLOCK 11
typedef unsigned long long DS_SOCKET;

#endif

namespace Version {
union ui64_ui8 {
    uint64_t ui64;
    uint8_t ui8[8];
};
static std::string to_string(const uint64_t &ui64);

namespace TcpIoBlock {
static const uint64_t gMinVersion = 0x01000001;  // version 1.0.0.1
//static const uint64_t gMaxVersion = 0x01020304;  // version 1.2.3.4
static bool check(const uint64_t &ui64);
} // namespace TcpIoBlock

namespace TcpClientLibrary {
static const uint64_t gVersion = 0x01000001;  // version 1.0.0.1
} // namespace TcpClientLibrary
}


/// The types of control tags
namespace ControlTags {
const uint64_t exit = 0x01;
const uint64_t socketconfig = 0x02;
const uint64_t terminate = 0xFFFFFFFFFF439EB2;
}

/// The types of data
namespace DataTypes {
const uint64_t data = 0x00;
const uint64_t service = 0x01;
const uint64_t padding = 0x02;
}

/// Enum of state of TCP connection
enum ConnectionState {
    Undefined,
    Initialized,
    Connected,
    Disconnected
};

/**
 * @brief Struct for storage of phisical info about TCP connection
 */
struct Connection {
    std::vector<DS_SOCKET> sockfd_;
    std::vector<DS_SOCKET> sockfd_send_;
    std::vector<DS_SOCKET> sockfd_rcv_;
    ConnectionState state_;
    struct hostent *server_;
    struct sockaddr_in server_addr_;
    size_t iSocketSend_;
    size_t iSocketRcv_;
    //----------------------------------------------------------------
    Connection() : state_(ConnectionState::Undefined), server_(nullptr), iSocketSend_(0), iSocketRcv_(0)
    {
        memset(&server_addr_, '\0', sizeof(server_addr_));
    }
};

/** @enum ConnectionInfo
 * @brief Struct for storage of info about connection
 */
struct ConnectionInfo {
    int port_;
    std::string remoteAddress_;
    uint32_t tcpBufSize_;
    bool displayRaw_;
    int delayRcvMs_;
    int delaySendMs_;
    uint8_t nSockets_;
    bool exit_;
    bool isDuplexSockets_;
    int delayAfterConnect_;
    int timeOut_;
    int waitConnect_;
    //------------------------------------------------
    ConnectionInfo() : port_(0), remoteAddress_(""), tcpBufSize_(128), displayRaw_(false), delayRcvMs_(0), delaySendMs_(0),
        nSockets_(1), exit_(false), isDuplexSockets_(false), delayAfterConnect_(0), timeOut_(0), waitConnect_(0)
    {
        ;
    }
    //------------------------------------------------
    bool operator==(const ConnectionInfo &rhs)
    {
        return (port_ == rhs.port_) && (remoteAddress_ == rhs.remoteAddress_);
    }
    bool operator!=(const ConnectionInfo &rhs)
    {
        return (port_ != rhs.port_) || (remoteAddress_ != rhs.remoteAddress_);
    }
};

/** \enum StatusInfo
 * @brief The status of send/receive operation
 */
enum StatusInfo : int {
    /// Unsupported cotrol tag
    UnsupportTag = 0,
    /// Read from socket error
    Error = 1,
    /// Read data processing
    DataProcessing = 2,
    /// Read data processing
    DataPadding = 3,
    /// All data is read
    AllData = 4,
    /// Termination control tag found
    Termination = 5,
    /// Operation finished (all data and tag of termination is received)
    Finish = 6
};

/**
 * @class TCPClient
 * @brief The TCPClient class for sending/receiving data to/from tcp_io_component
 * @par Simple example for using this class see below
 * @par This example send a string @c "Hello world!" to host=@c host with port=@c port
 * @code
 * bool simpleExample(const std::string& host, const int port)
 * {
 *     TCPClient client;
 *     ConnectionInfo connectionInfo;
 *     connectionInfo.remoteAddress_ = host;
 *     connectionInfo.port_ = port;
 *     if (!client.initConnection(connectionInfo)) {
 *        std::cerr << "Init error\n";
 *        return false;
 *     }
 *     if (!client.connect()) {
 *         std::cerr << "Connect error\n";
 *         return false;
 *     }
 *     std::string send_data("Hello world!");
 *     auto sent_bytes = client.send_to_tcp_io(send_data.c_str(), static_cast<int>(send_data.size()));
 *     std::cout << "Sent " << sent_bytes << " bytes Data bytes: " << send_data.size() << std::endl;
 *     client.send_terminate();
 *     std::vector<char> rcv_data(send_data.length() + 7);
 *     auto rcv_bytes = client.receive_tcp_io(&rcv_data.front(), static_cast<int>(rcv_data.size()));
 *     std::cout << "Reseived " << rcv_bytes << ":\n" << std::string(&rcv_data.front(), static_cast<size_t>(rcv_bytes)) << "\n";
 *     return true;
 * }
 * @endcode
 */
class TCPClient
{
public:
    static const uint64_t gVersion = 0x01000001;
    /// Consructor
    TCPClient();
    /// Destructor
    ~TCPClient();
public:
    /**
     * @brief Initialization of parameters for connect from struct "info".
     * @param info
     * @return true if initialization finished is success, false else.
     */
    bool initConnection(const ConnectionInfo &info);
    /**
     * @brief Configure the TCPClient from tcp_io_block
     * @return true if parameters from server is set, false if else
     */
    bool configSocket(const size_t &idx_socket, const DS_SOCKET &sock, size_t &socketID);
    /**
     * @brief Open TCP connection with server
     * @return true if conect is on, false if else
     */
    bool connect();
    /**
     * @brief Close TCP connection with server
     * @return true if disconect finished successfully, false if else
     */
    bool disconnect();
    /**
     * @brief Get state of connection
     * @return  ConnectionState
     */
    ConnectionState getState() const;
    /**
     * @brief Send data to tcp_io component
     * @param dataIn - pointer to data
     * @param lengthIn - length of data
     * @return -1 - if error, else count of sending bytes
     */
    int send_to_tcp_io(const char *dataIn, const int lengthIn);
    /**
     * @brief Send terminate tag to server
     * @return -1 - if error, else count of sending bytes
     */
    int send_terminate();
    /**
     * @brief Send terminate ant exit tags to server
     * @return -1 - if error, else count of sending bytes
     */
    int send_terminate_exit();
    /**
     * @brief Send data to tcp server
     * @param data - pointer to data
     * @param length - length of data
     * @return -1 - if error, else count of sending bytes
     */
    int send(const char *data, int length);
    /**
     * @brief Send \"length\" bytes of data to TCP server
     * @param data - pointer to data for reaading
     * @param length - size of buffer for data
     * @return -1 - if error, else counter of the sending bytes
     */
    int send_need(char *data, int length);
    /**
     * @brief Receive data from TCP server
     * @param data - pointer to data for reaading
     * @param length - size of buffer for data
     * @return -1 - if error, else counter of the received bytes
     */
    int receive_from_socket(const DS_SOCKET &socket, char *data, const int length);
    int receive_data(char *data, int length, bool &is_terminate);
    int receive(char *data, int length);
    /**
     * @brief Receive \"length\" bytes of data from TCP server
     * @param data - pointer to data for reaading
     * @param length - size of buffer for data
     * @return -1 - if error, else counter of the received bytes
     */
    int receive_need(char *data, int length);
    /**
     * @brief Receive data from tcp_io component
     * @param data - pointer to data for reaading
     * @param length - size of buffer for data
     * @return -1 - if error, else counter of the received bytes
     */
    int receive_tcp_io(char *data, int length);
    /**
     * @brief Display data in hex format
     * @param data - pointer to data
     * @param len - size of buffer of data
     */
    void display_data(const char *data, size_t len, const std::string &strTitle = "Raw buffer");
    /**
     * @brief Send data to tcp_io component
     * @param data - vector of data
     * @return -1 - if error, else count of sending bytes
     */
    int send_sync(const std::vector<char> &data);
    /**
     * @brief Receive data from tcp_io component
     * @param data - vector(buffer) for reaading
     * @return -1 - if error, else counter of the received bytes
     */
    int receive_sync(std::vector<char> &data);
    void set_display(const bool displayRaw);
    ConnectionInfo &get_connection_info()
    {
        return connectionInfo_;
    }
    Connection &get_connection()
    {
        return connection_;
    }
    uint64_t get_bytes_sent() const
    {
        return bytesSent_;
    }
    uint64_t get_bytes_received() const
    {
        return bytesReceived_;
    }
    double get_sent_time_ms() const
    {
        auto ms = (std::chrono::duration<uint64_t, std::nano>(sentTime_)).count();
        return double(ms / 1000000);
    }
    double get_received_time_ms() const
    {
        auto ms = std::chrono::duration<uint64_t, std::nano>(receivedTime_).count();
        return double(ms / 1000000);
    }
    void SleepMs(int sleepMs);
    int get_error();
    static void out_str(const std::string &str, std::ostream &stream);

    uint64_t bytesSent_;
    uint64_t bytesReceived_;
private:
    const int wait_step_sec = 5;
    void set_error(const std::string &msg);
    //void set_error(const int &error, const std::string &msg);
    std::string getHostByName(const std::string &host) const;
    void setTimeout(const DS_SOCKET sock, const bool is_receive, long to);
    void setTimeout(long to);
    /// Struct for storage of info about connection
    ConnectionInfo connectionInfo_;
    /// Struct for storage of phisical info about TCP connection
    Connection connection_;
    //std::vector<char> tcpSendVec_;
    //std::vector<char> tcpRcvVec_;
    //std::vector<char> tcpTmpBuf_;
    static std::mutex mutex_;
    uint64_t sentTime_;
    uint64_t receivedTime_;
    int lastError_;
//    std::chrono::duration<double, std::milli> sentTimeMs_;
//    std::chrono::duration<double, std::milli> receivedTimeMs_;
};

/*!
 * @class ReceiveChunks
 * @brief For receiving data in several chunks
 * @par Example for using this class see below
 * @code
 * bool receiveFileChunk(TCPClient& client, const std::string& fileNameReceive)
 * {
 *     std::ofstream ofs(fileNameReceive, std::ofstream::out | std::ios::binary);
 *     if (!ofs.is_open()) {
 *         std::cerr << "File \"" << std::string(fileNameReceive) << "\" not created.\n";
 *         return;
 *     }
 *     int rcv_bytes(-1);
 *     int rcv_all_bytes(0);
 *     int rcv_block_count(0);
 *     std::vector<char> rcv_data(BUF_SIZE);
 *     ReceiveChunks rc(&client);
 *     while (rc.getStatus() == StatusInfo::DataProcessing) {
 *         rcv_bytes = rc.receive_data_block(&rcv_data.front(), static_cast<int>(rcv_data.size()));
 *         if (rcv_bytes <= 0) {
 *             break;
 *         }
 *         ofs.write(&rcv_data.front(), rcv_bytes);
 *         if (!ofs.good()) {
 *             std::cerr << "Write to file error\n";
 *             break;
 *         }
 *         rcv_all_bytes += rcv_bytes;
 *         std::cout << "Received " << rcv_all_bytes << " bytes\n";
 *     }
 *     return true;
 * }
 * @endcode
 */
class ReceiveChunks
{
public:
    /// Constructor
    ReceiveChunks(TCPClient *client);
    /// Destructor
    ~ReceiveChunks();
    /**
     * @brief Getting a status of operation of receiving
     * @return StatusInfo
     */
    StatusInfo getStatus();
    /// Get a size of data
    uint64_t getDataSize();
    /**
     * @brief Receive block of data size of length
     * @param[in] data - input buffer
     * @param[in] length - size of input buffer
     * @return <table>
     * <tr> <td> Value </td>        <td>Description</td> </tr>
     * <tr> <td>---------</td>      <td>---------</td> </tr>
     * <tr> <td> `-1` </td>         <td> Reading error from a sockett </td> </tr>
     * <tr> <td> `0` </td>          <td> There are no data for reading </td> </tr>
     * <tr> <td> `writebytes` </td> <td> It is read from a socket of bytes </td> </tr>
     * </table>
     */
    int receive_data_block(char *data, int length);
private:
    /// Receiving a header of data and set the dataSize and status
    StatusInfo receive_header();

private:
    /// Pointer to TCPCLient class
    TCPClient *client_;
    /// The size of data for receiving
    uint64_t dataSize_;
    /// The counter of bytes which need to be received
    uint64_t needBytesRecieve_;
    /// Status of operation the receiving of data
    StatusInfo status_;
};

/**
 * @class SendChunks
 * @brief For sending data in several chunks
 * @par Example for using this class see below
 * @code
 * void sendFileChunks(TCPClient &client, const std::string &fileName)
 * {
 *     std::ifstream ifs(fileName, std::ios::binary | std::ios::ate);
 *     if (!ifs) {
 *         std::cerr << "File \"" << std::string(fileName) << "\" not found.\n";
 *         return;
 *     }
 *     auto fileSize = ifs.tellg();
 *     ifs.seekg(std::ios::beg);
 *     std::vector<char> send_data(BUF_SIZE);
 *     int send_block_count(0);
 *     int sent_bytes(0);
 *     SendChunks sc(&client, fileSize);
 *     while (sc.getStatus() == StatusInfo::DataProcessing) {
 *         ifs.read(&send_data.front(), send_data.size());
 *         auto read_bytes = ifs.gcount();
 *         sent_bytes += sc.send_data_block(&send_data.front(), static_cast<int>(read_bytes));
 *         std::cout << "Sent all " << sent_bytes << " bytes\n";
 *     }
 *     ifs.close();
 * }
 * @endcode
 */
class SendChunks
{
public:
    /// Constructor
    SendChunks(TCPClient *client, const uint64_t dataSize, const uint64_t blockSize = 128);
    /// Destructor
    virtual ~SendChunks();
    /**
     * @brief Getting a status of operation of receiving
     * @return StatusInfo
     */
    StatusInfo getStatus();
    /// Get a size of data
    uint64_t getDataSize();
    /**
     * @brief Sending data unit of the given length
     * @param data - input buffer
     * @param length - size of input buffer
     * @return <table>
     * <tr><td> Value </td><td>Description</td></tr>
     * <tr><td>---------</td><td>---------</td></tr>
     * <tr><td> `-1` </td><td> Sending error to a socket </td></tr>
     * <tr><td> `0` </td><td> There are no data to sending </td></tr>
     * <tr><td> `writebytes` </td><td> It is write to socket of bytes </td></tr>
     * </table>
     */
    virtual int send_data_block(char *data, int length);
    /// The sending of header of portion of data
    virtual int send_header();

protected:
    /// Pointer to TCPCLient class
    TCPClient *client_;
    /// The size of data for sending
    uint64_t dataSize_;
    /// The counter of bytes which need to be send
    uint64_t needBytesSend_;
    /// Status of operation the receiving of data
    StatusInfo status_;
    uint64_t blockSize_;
};

/**
 * @class SendBlocks
 * @brief For sending data in several block of fixed size
 * @par Example for using this class see below
 * @code
 * void sendFileBlocks(TCPClient &client, const std::string &fileName, const unsigned int blockSize)
 * {
 *     std::ifstream ifs(fileName, std::ios::binary | std::ios::ate);
 *     if (!ifs) {
 *         std::cerr << "File \"" << std::string(fileName) << "\" not found.\n";
 *         return;
 *     }
 *     auto fileSize = ifs.tellg();
 *     ifs.seekg(std::ios::beg);
 *     std::vector<char> send_data(blockSize);
 *     int send_block_count(0);
 *     int sent_bytes(0);
 *     SendBlocks sb(&client, fileSize, blockSize);
 *     while (sb.getStatus() == StatusInfo::DataProcessing) {
 *         ifs.read(&send_data.front(), send_data.size());
 *         auto read_bytes = ifs.gcount();
 *         sent_bytes += sb.send_data_block(&send_data.front(), static_cast<int>(read_bytes));
 *         std::cout << "Sent all " << sent_bytes << " bytes\n";
 *     }
 *     ifs.close();
 * }
 * @endcode
 */
class SendBlocks : public SendChunks
{
public:
    /// Constructor
    SendBlocks(TCPClient *client, const uint64_t dataSize, const unsigned int blockSize);

    /**
     * @brief Sending data unit of the given length
     * @param data - input buffer
     * @param length - size of input buffer
     * @return <table>
     * <tr><td> Value </td><td>Description</td></tr>
     * <tr><td>---------</td><td>---------</td></tr>
     * <tr><td> `-1` </td><td> Sending error to a socket </td></tr>
     * <tr><td> `0` </td><td> There are no data to sending </td></tr>
     * <tr><td> `writebytes` </td><td> It is write to socket of bytes </td></tr>
     * </table>
     */
    // virtual int send_data_block(char *data, int length);
    /// The sending of header of portion of data
    virtual int send_header();

protected:
    /// The size of block of data
    unsigned int blockSize_;
    /// The count of the sent bytes of data
    uint64_t bytesCount_;
};

#endif // TCPCLIENT_H
