# class `ReceiveChunks` 


For receiving data in several chunks.

Example for using this class see below
```cpp
bool receiveFileChunk(TCPClient& client, const std::string& fileNameReceive)
{
    std::ofstream ofs(fileNameReceive, std::ofstream::out | std::ios::binary);
    if (!ofs.is_open()) {
        std::cerr << "File \"" << std::string(fileNameReceive) << "\" not created.\n";
        return;
    }
    int rcv_bytes(-1);
    int rcv_all_bytes(0);
    int rcv_block_count(0);
    std::vector<char> rcv_data(BUF_SIZE);
    ReceiveChunks rc(&client);
    while (rc.getStatus() == StatusInfo::DataProcessing) {
        rcv_bytes = rc.receive_data_block(&rcv_data.front(), static_cast<int>(rcv_data.size()));
        if (rcv_bytes <= 0) {
            break;
        }
        ofs.write(&rcv_data.front(), rcv_bytes);
        if (!ofs.good()) {
            std::cerr << "Write to file error\n";
            break;
        }
        rcv_all_bytes += rcv_bytes;
        std::cout << "Received " << rcv_all_bytes << " bytes\n";
    }
    return true;
}
```

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  ReceiveChunks(`[`TCPClient`](#classTCPClient)` * client)` | Constructor.
`public  ~ReceiveChunks()` | Destructor.
`public `[`StatusInfo`](#tcpclient_8h_1a470e2132067e955a1f066c9c4602d6d9)` getStatus()` | Getting a status of operation of receiving.
`public uint64_t getDataSize()` | Get a size of data.
`public int receive_data_block(char * data,int length)` | Receive block of data size of length.

## Members

#### `public  ReceiveChunks(`[`TCPClient`](#classTCPClient)` * client)` 

Constructor.



#### `public  ~ReceiveChunks()` 

Destructor.



#### `public `[`StatusInfo`](#tcpclient_8h_1a470e2132067e955a1f066c9c4602d6d9)` getStatus()` 

Getting a status of operation of receiving.

#### Returns
StatusInfo

#### `public uint64_t getDataSize()` 

Get a size of data.



#### `public int receive_data_block(char * data,int length)` 

Receive block of data size of length.

#### Parameters
* `data` - input buffer 


* `length` - size of input buffer 





#### Returns

Value  |Description
------|------
`-1`|Reading error from a sockett
`0`|There are no data for reading
`writebytes`|It is read from a socket of bytes

# class `SendBlocks` 

```
class SendBlocks
  : public SendChunks
```  

For sending data in several block of fixed size.

Example for using this class see below
```cpp
void sendFileBlocks(TCPClient &client, const std::string &fileName, const unsigned int blockSize)
{
    std::ifstream ifs(fileName, std::ios::binary | std::ios::ate);
    if (!ifs) {
        std::cerr << "File \"" << std::string(fileName) << "\" not found.\n";
        return;
    }
    auto fileSize = ifs.tellg();
    ifs.seekg(std::ios::beg);
    std::vector<char> send_data(blockSize);
    int send_block_count(0);
    int sent_bytes(0);
    SendBlocks sb(&client, fileSize, blockSize);
    while (sb.getStatus() == StatusInfo::DataProcessing) {
        ifs.read(&send_data.front(), send_data.size());
        auto read_bytes = ifs.gcount();
        sent_bytes += sb.send_data_block(&send_data.front(), static_cast<int>(read_bytes));
        std::cout << "Sent all " << sent_bytes << " bytes\n";
    }
    ifs.close();
}
```

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  SendBlocks(`[`TCPClient`](#classTCPClient)` * client,const uint64_t dataSize,const unsigned int blockSize)` | Constructor.
`public virtual int send_data_block(char * data,int length)` | Sending data unit of the given length.
`public virtual int send_header()` | The sending of header of portion of data.
`protected unsigned int blockSize_` | The size of block of data.
`protected uint64_t bytesCount_` | The count of the sent bytes of data.

## Members

#### `public  SendBlocks(`[`TCPClient`](#classTCPClient)` * client,const uint64_t dataSize,const unsigned int blockSize)` 

Constructor.



#### `public virtual int send_data_block(char * data,int length)` 

Sending data unit of the given length.

#### Parameters
* `data` - input buffer 


* `length` - size of input buffer 





#### Returns

Value |Description
------|------
`-1`|Sending error to a socket
`0`|There are no data to sending
`writebytes`|It is write to socket of bytes

#### `public virtual int send_header()` 

The sending of header of portion of data.



#### `protected unsigned int blockSize_` 

The size of block of data.



#### `protected uint64_t bytesCount_` 

The count of the sent bytes of data.



# class `SendChunks` 


For sending data in several chunks.

Example for using this class see below
```cpp
void sendFileChunks(TCPClient &client, const std::string &fileName)
{
    std::ifstream ifs(fileName, std::ios::binary | std::ios::ate);
    if (!ifs) {
        std::cerr << "File \"" << std::string(fileName) << "\" not found.\n";
        return;
    }
    auto fileSize = ifs.tellg();
    ifs.seekg(std::ios::beg);
    std::vector<char> send_data(BUF_SIZE);
    int send_block_count(0);
    int sent_bytes(0);
    SendChunks sc(&client, fileSize);
    while (sc.getStatus() == StatusInfo::DataProcessing) {
        ifs.read(&send_data.front(), send_data.size());
        auto read_bytes = ifs.gcount();
        sent_bytes += sc.send_data_block(&send_data.front(), static_cast<int>(read_bytes));
        std::cout << "Sent all " << sent_bytes << " bytes\n";
    }
    ifs.close();
}
```

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  SendChunks(`[`TCPClient`](#classTCPClient)` * client,const uint64_t dataSize)` | Constructor.
`public virtual  ~SendChunks()` | Destructor.
`public `[`StatusInfo`](#tcpclient_8h_1a470e2132067e955a1f066c9c4602d6d9)` getStatus()` | Getting a status of operation of receiving.
`public uint64_t getDataSize()` | Get a size of data.
`public virtual int send_data_block(char * data,int length)` | Sending data unit of the given length.
`public virtual int send_header()` | The sending of header of portion of data.
`protected `[`TCPClient`](#classTCPClient)` * client_` | Pointer to TCPCLient class.
`protected uint64_t dataSize_` | The size of data for sending.
`protected uint64_t needBytesSend_` | The counter of bytes which need to be send.
`protected `[`StatusInfo`](#tcpclient_8h_1a470e2132067e955a1f066c9c4602d6d9)` status_` | Status of operation the receiving of data.

## Members

#### `public  SendChunks(`[`TCPClient`](#classTCPClient)` * client,const uint64_t dataSize)` 

Constructor.



#### `public virtual  ~SendChunks()` 

Destructor.



#### `public `[`StatusInfo`](#tcpclient_8h_1a470e2132067e955a1f066c9c4602d6d9)` getStatus()` 

Getting a status of operation of receiving.

#### Returns
StatusInfo

#### `public uint64_t getDataSize()` 

Get a size of data.



#### `public virtual int send_data_block(char * data,int length)` 

Sending data unit of the given length.

#### Parameters
* `data` - input buffer 


* `length` - size of input buffer 





#### Returns

Value |Description
------|------
`-1`|Sending error to a socket
`0`|There are no data to sending
`writebytes`|It is write to socket of bytes

#### `public virtual int send_header()` 

The sending of header of portion of data.



#### `protected `[`TCPClient`](#classTCPClient)` * client_` 

Pointer to TCPCLient class.



#### `protected uint64_t dataSize_` 

The size of data for sending.



#### `protected uint64_t needBytesSend_` 

The counter of bytes which need to be send.



#### `protected `[`StatusInfo`](#tcpclient_8h_1a470e2132067e955a1f066c9c4602d6d9)` status_` 

Status of operation the receiving of data.



# class `TCPClient` 


The [TCPClient](#classTCPClient) class for sending/receiving data to/from tcp_io_component.

Simple example for using this class see below

This example send a string `"Hello world!"` to host=`host` with port=`port`
```cpp
bool simpleExample(const std::string& host, const int port)
{
    TCPClient client;
    ConnectionInfo connectionInfo;
    connectionInfo.remoteAddress_ = host;
    connectionInfo.port_ = port;
    if (!client.initConnection(connectionInfo)) {
       std::cerr << "Init error\n";
       return false;
    }
    if (!client.connect()) {
        std::cerr << "Connect error\n";
        return false;
    }
    std::string send_data("Hello world!");
    auto sent_bytes = client.send_to_tcp_io(send_data.c_str(), static_cast<int>(send_data.size()));
    std::cout << "Sent " << sent_bytes << " bytes Data bytes: " << send_data.size() << std::endl;
    client.send_terminate();
    std::vector<char> rcv_data(send_data.length() + 7);
    auto rcv_bytes = client.receive_tcp_io(&rcv_data.front(), static_cast<int>(rcv_data.size()));
    std::cout << "Reseived " << rcv_bytes << ":\n" << std::string(&rcv_data.front(), static_cast<size_t>(rcv_bytes)) << "\n";
    return true;
}
```

## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public  TCPClient()` | Consructor.
`public  ~TCPClient()` | Destructor.
`public bool initConnection(const `[`ConnectionInfo`](#structConnectionInfo)` & info)` | Initialization of parameters for connect from struct "info".
`public bool connect()` | Open TCP connection with server.
`public bool disconnect()` | Close TCP connection with server.
`public `[`ConnectionState`](#tcpclient_8h_1acdd867d72142510ce53521a63a062f9b)` getState() const` | Get state of connection.
`public int send_to_tcp_io(const char * dataIn,const int lengthIn)` | Send data to tcp_io component.
`public int send_terminate()` | Send terminate tag to server.
`public int send_terminate_exit()` | Send terminate ant exit tags to server.
`public int send_block_size(const unsigned int blockSize)` | Send size of block to server.
`public int send(const char * data,int length)` | Send data to tcp server.
`public int send_need(char * data,int length)` | Send "length" bytes of data to TCP server.
`public int receive(char * data,int length)` | Receive data from TCP server.
`public int receive_need(char * data,int length)` | Receive "length" bytes of data from TCP server.
`public int receive_tcp_io(char * data,int length)` | Receive data from tcp_io component.
`public void display_data(char * data,size_t len)` | Display data in hex format.

## Members

#### `public  TCPClient()` 

Consructor.



#### `public  ~TCPClient()` 

Destructor.



#### `public bool initConnection(const `[`ConnectionInfo`](#structConnectionInfo)` & info)` 

Initialization of parameters for connect from struct "info".

#### Parameters
* `info` 





#### Returns
true if initialization finished is success, false else.

#### `public bool connect()` 

Open TCP connection with server.

#### Returns
true if conect is on, false if else

#### `public bool disconnect()` 

Close TCP connection with server.

#### Returns
true if disconect finished successfully, false if else

#### `public `[`ConnectionState`](#tcpclient_8h_1acdd867d72142510ce53521a63a062f9b)` getState() const` 

Get state of connection.

#### Returns
ConnectionState

#### `public int send_to_tcp_io(const char * dataIn,const int lengthIn)` 

Send data to tcp_io component.

#### Parameters
* `dataIn` - pointer to data 


* `lengthIn` - length of data 





#### Returns
-1 - if error, else count of sending bytes

#### `public int send_terminate()` 

Send terminate tag to server.

#### Returns
-1 - if error, else count of sending bytes

#### `public int send_terminate_exit()` 

Send terminate ant exit tags to server.

#### Returns
-1 - if error, else count of sending bytes

#### `public int send_block_size(const unsigned int blockSize)` 

Send size of block to server.

#### Parameters
* `size` of block data 





#### Returns
-1 - if error, else count of sending bytes

#### `public int send(const char * data,int length)` 

Send data to tcp server.

#### Parameters
* `data` - pointer to data 


* `length` - length of data 





#### Returns
-1 - if error, else count of sending bytes

#### `public int send_need(char * data,int length)` 

Send "length" bytes of data to TCP server.

#### Parameters
* `data` - pointer to data for reaading 


* `length` - size of buffer for data 





#### Returns
-1 - if error, else counter of the received bytes

#### `public int receive(char * data,int length)` 

Receive data from TCP server.

#### Parameters
* `data` - pointer to data for reaading 


* `length` - size of buffer for data 





#### Returns
-1 - if error, else counter of the received bytes

#### `public int receive_need(char * data,int length)` 

Receive "length" bytes of data from TCP server.

#### Parameters
* `data` - pointer to data for reaading 


* `length` - size of buffer for data 





#### Returns
-1 - if error, else counter of the received bytes

#### `public int receive_tcp_io(char * data,int length)` 

Receive data from tcp_io component.

#### Parameters
* `data` - pointer to data for reaading 


* `length` - size of buffer for data 





#### Returns
-1 - if error, else counter of the received bytes

#### `public void display_data(char * data,size_t len)` 

Display data in hex format.

#### Parameters
* `data` - pointer to data 


* `len` - size of buffer of data

# struct `Connection` 


Struct for storage of phisical info about TCP connection.



## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public long long sockfd_` | 
`public `[`ConnectionState`](#tcpclient_8h_1acdd867d72142510ce53521a63a062f9b)` state_` | 
`public struct hostent * server_` | 
`public struct sockaddr_in server_addr_` | 
`public inline  Connection()` | 

## Members

#### `public long long sockfd_` 





#### `public `[`ConnectionState`](#tcpclient_8h_1acdd867d72142510ce53521a63a062f9b)` state_` 





#### `public struct hostent * server_` 





#### `public struct sockaddr_in server_addr_` 





#### `public inline  Connection()` 





# struct `ConnectionInfo` 






## Summary

 Members                        | Descriptions                                
--------------------------------|---------------------------------------------
`public int port_` | 
`public std::string remoteAddress_` | 
`public inline  ConnectionInfo()` | 
`public inline bool operator==(const `[`ConnectionInfo`](#structConnectionInfo)` & rhs)` | 
`public inline bool operator!=(const `[`ConnectionInfo`](#structConnectionInfo)` & rhs)` | 

## Members

#### `public int port_` 





#### `public std::string remoteAddress_` 





#### `public inline  ConnectionInfo()` 





#### `public inline bool operator==(const `[`ConnectionInfo`](#structConnectionInfo)` & rhs)` 





#### `public inline bool operator!=(const `[`ConnectionInfo`](#structConnectionInfo)` & rhs)` 





