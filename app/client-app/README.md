# OPTIONS:

```
test_app {OPTIONS}

-h, --help                Display this help menu
-w[wait_connect]          Sleep of <wait_connect> ms after each connection.
-b[block_size]            TCP Buffer size in bytes. Default is 4096
Commands:
  -s                        send data
  -c                        convert file
Required send options:
  -s                        send data
  -n[hostname]              The name (ip addr) of host.
  -p[port]                  The number of port. Default is 12340.
  Data options:
    -r[data_length]           Send random data length of data_length. Default is 4096
    -d[data_string]           Send dataString
    -f[data_file]             Send data from file filename. Data retrieved save to file fileName+".out"
    -T[test_case]             Run test case:
                              [1..7[:<sNblock>:<rNblock>]|8[:<sNblock>:<rNblock>:<delayClocks>]|exit|all|all_async]
    -e                        Sending event for getting status
    -t                        Terminate the server
  --n_sockets [nSockets]    Quantity of sockets for connection.
Send options:
  --type [type]             Type of input file
  Type of socket using:
    --tcp_io_block            Type of using socket.
    --tcp_io_block_x2         Type of using socket.
  --duplex                  Type of using socket.
  --dS [msec]               Delay after every sending of block of data, milliseconds. Default is 0
  --dR [msec]               Delay after every receiving of block of data, milliseconds. Default is 0
  --time_out [sec]          Time out for wait send/receive operations, seconds. Default is 0
  -P                        Turn on print option
  -t                        Terminate the server
Required convert options:
  -c                        convert file
  -i[file], --ifile [file]  Name of input file
  --itype [type]            Type of input file
  -o[file], --ofile [file]  Name of output file
  --otype [type]            Type of output file
Convert options:
  --tag                     Output data with tag (used with '--otype hex' parameter only)
```

##    Test cases:
Parameter | Description
----------|--------------------
1         | Validate consistent endianess
2         | TCP Echo
3         | Back pressure
4         | Send pressure
5         | Send huge - Receive Huge
6         | Close socket by client during server send
7         | Close socket by client during server receive
8         | Send huge with delay on server - Receive small
exit      | Server exit
all       | Run all Test cases 1-7
all_async | Run all Test cases 1-7 async
etl       | Run test for ETL app
sabre     | Run test for Sabre app
pixia     | Run test for Pixia app

##    Type of file:
    bin: Any file
    hex: Text file, in wich every line is a word of size of 8 bytes in hex format
    ds8: Special format of file for sending to tcp_io_block component without processing. Used '-b<block_size>' parameter


# Examples
## Sending file (ds8-type) and exit command
./test_app -s -n appdev.test.directstream.com -p 10028 -f test01.dat -t

## Run validate consistent endianess
./test_app -s -n ec2-54-209-112-92.compute-1.amazonaws.com -p 10111 -T 1

## Run TCP echo
./test_app -s -n ec2-54-209-112-92.compute-1.amazonaws.com -p 10111 -T 2

# Build Instructions
For build of client need run next two commands:
```
cmake .
make
```
