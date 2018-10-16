#include <iostream>
#include <vector>
#include <sstream>
#include <iterator>
#include <iomanip>
#include <chrono>
#include <future>
#include <thread>
#include <fstream>
#include <algorithm>

// Include Argument Parser
#include "args.hxx"
// Include TCP client
#include "tcpclient.h"
#include "tcpclientapp.h"


uint64_t receiveQuickData(TCPClient &client, std::vector<uint64_t> &dataOut)
{
    auto bufSize = client.get_connection_info().tcpBufSize_;
    std::vector<uint64_t> data(static_cast<size_t>(bufSize / 8));
    auto pDataChar = reinterpret_cast<char *>(&data[0]);
    uint64_t iBlock(0);
    bool exit(false);
    utils::Timing tm("Receiving");
    do {
        int bytes = client.receive(pDataChar, bufSize);
        if (bytes < 0) {
            break;
        }
        if (DataTypes::data == data[1]) {
            dataOut.insert(dataOut.end(), data.begin() + 2, data.begin() + 2 + (data[0] + 7) / 8);
        }
        ++iBlock;
        exit = exit || (DataTypes::service == data[1] && (ControlTags::terminate == data[2]
                                                          || ControlTags::terminate == data[3]));
    } while (!exit);
    tm.outStr("Received blocks: " + std::to_string(iBlock));
    tm.outResultStr(iBlock * static_cast<uint64_t>(bufSize));

    return iBlock;
}


void statusOut(std::vector<std::future<uint64_t>> &f_vec, TCPClient &client, const long long interval = 5)
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
}

int main(int argc, char *argv[])
{
    int error(0);

    std::string appName(argv[0]);
    appName = appName.substr(appName.find_last_of("\\/:") + 1);

//    std::ofstream logFile;
//    std::ofstream logErrFile;
//    const std::vector<std::string> args(argv + 1, argv + argc);
//    if (args.end() != std::find(args.begin(), args.end(), "-l")) {
//        logFile.open(appName + ".log");
//        if (logFile.is_open()) {
//            std::cout.rdbuf(logFile.rdbuf());
//        }
//        logErrFile.open(appName + ".err");
//        if (logErrFile.is_open()) {
//            std::cerr.rdbuf(logErrFile.rdbuf());
//        }
//    }

    args::ArgumentParser args_parser("");
    args_parser.helpParams.width = 130;
    args_parser.helpParams.helpindent = 32;
    args_parser.Prog(appName);
    args_parser.LongSeparator(" ");
    args::HelpFlag help(args_parser, "help", "Display this help menu", { 'h', "help" });
    args::ValueFlag<int> delay_after_connect(args_parser, "delay_after_connect", "Sleep of <xxx> ms after each connection.", { 'w' },
                                             0);
    args::ValueFlag<uint32_t> blockSize(args_parser, "block_size", "TCP Buffer size in bytes. Default is " + std::to_string(4096), { 'b' },
                                        4096);
    args::Group g_cmds(args_parser, "Commands:", args::Group::Validators::Xor);
    args::Flag cmd_send(g_cmds, "send", "send data", { 's' });
    args::Flag cmd_convert(g_cmds, "convert", "convert file", { 'c' });

    // send options required
    args::Group g_send_required(args_parser, "Required send options:", args::Group::Validators::AllOrNone);
    g_send_required.Add(cmd_send);
    args::ValueFlag<std::string> host(g_send_required, "hostname", "The name (ip addr) of host.", { 'n' });
    args::ValueFlag<int> port(g_send_required, "port", "The number of port. Default is 12340.", { 'p' }, 12340);
    // data options
    args::Group g_data(g_send_required, "Data options:", args::Group::Validators::AtLeastOne);
    args::ValueFlag<std::string> data_file(g_data, "data_file",
                                           "Send data from file filename. Data retrieved save to file fileName+\".out\"", { 'f' });
    // send options
    args::Group g_send(args_parser, "Send options:", args::Group::Validators::DontCare);
    args::MapFlag<std::string, utils::convertors::FileType> type(g_send, "type", "Type of input file", { "type" },
                                                                 utils::convertors::typesOfFile);
    args::Flag old_ignored_01(g_send, "tcp_io_block", "Old parameter, now ignored.", { "tcp_io_block" });
    args::Flag old_ignored_02(g_send, "tcp_io_block_x2", "Old parameter, now ignored.", { "tcp_io_block_x2" });
    args::Flag duplex(g_send, "duplexSockets", "Type of using socket.", { "duplex" });
    args::ValueFlag<int> nSockets(g_send_required, "nSockets", "Quantity of sockets for connection.", { "n_sockets" });
    args::ValueFlag<int> delaySend(g_send, "msec", "Delay after every sending of block of data, milliseconds. Default is 0", { "dS" },
                                   0);
    args::ValueFlag<int> delayReceive(g_send, "msec", "Delay after every receiving of block of data, milliseconds. Default is 0", { "dR" },
                                      0);
    args::ValueFlag<int> timeOut(g_send, "sec", "Time out for wait send/receive operations, seconds. Default is 0", { "time_out" },
                                 0);
    args::ValueFlag<int> wait_connect(g_send, "sec", "Waiting of TCP connection, seconds. Default is 0", { "wait_connect" }, 0);
    args::Flag print(g_send, "print", "Turn on print option", { 'P' });
    args::Flag term(g_send, "term", "Terminate the server", { 't' });
    g_data.Add(term);

    // convert options required
    args::Group g_convert_required(args_parser, "Required convert options:",
                                   args::Group::Validators::AllOrNone);
    g_convert_required.Add(cmd_convert);
    args::ValueFlag<std::string> ifile(g_convert_required, "file", "Name of input file", { 'i', "ifile" });
    args::MapFlag<std::string, utils::convertors::FileType> itype(g_convert_required, "type",
                                                                  "Type of input file", { "itype" },
                                                                  utils::convertors::typesOfFile);
    args::ValueFlag<std::string> ofile(g_convert_required, "file", "Name of output file", { 'o', "ofile" });
    args::MapFlag<std::string, utils::convertors::FileType> otype(g_convert_required, "type",
                                                                  "Type of output file", { "otype" },
                                                                  utils::convertors::typesOfFile);
    // convert options
    args::Group g_convert(args_parser, "Convert options:", args::Group::Validators::DontCare);
    args::Flag tag(g_convert, "tag", "Output data with tag (used with '--otype hex' parameter only)", { "tag" });

    args_parser.Epilog(
        "-------------\n"
        "Type of file:\n"
        "-------------\n"
        "bin: Any file\n"
        "hex: Text file, in wich every line is a word of size of 8 bytes in hex format\n"
        "ds8: Special format of file for sending to tcp_io_block component without processing. Used '-b<block_size>' parameter"
    );

    do {
        try {
            args_parser.ParseCLI(argc, argv);
            if (!duplex && nSockets.Get() > 1 && nSockets.Get() % 2) {
                throw (args::ValidationError("Wrong parameters of sockets."));
            }
        } catch (args::Help) {
            std::cout << args_parser;
            error = 0;
            break;
        } catch (args::ParseError e) {
            std::cerr << "Not valid input argument(s).\n" << e.what() << std::endl;
            std::cerr << "\nUse ./" << appName << " -h for help." << std::endl;
            error = 2;
            break;
        } catch (args::ValidationError e) {
            std::cerr << "Not valid input argument(s).\n" << e.what() << std::endl;
            //parser.Help(std::cerr);
            std::cerr << "\nUse ./" << appName << " -h for help." << std::endl;
            error = 3;
            break;
        }

        if (cmd_send) {
            // Setup TCP client
            ConnectionInfo connectionInfo;
            connectionInfo.remoteAddress_ = host.Get();
            connectionInfo.port_ = port.Get();
            connectionInfo.tcpBufSize_ = blockSize.Get();
            connectionInfo.displayRaw_ = print.Get();
            connectionInfo.delayRcvMs_ = delayReceive.Get();
            connectionInfo.delaySendMs_ = delaySend.Get();
            connectionInfo.nSockets_ = std::max<int>(1, nSockets.Get());
            connectionInfo.exit_ = term;
            connectionInfo.isDuplexSockets_ = duplex || connectionInfo.nSockets_ == 1;
            connectionInfo.delayAfterConnect_ = delay_after_connect.Get();
            connectionInfo.timeOut_ = timeOut.Get();
            connectionInfo.waitConnect_ = wait_connect.Get();
            if (data_file) {
                std::string dataFile(data_file.Get());
                std::string dataFileOut("");
                if ("STDIN" == utils::str_to_upper(dataFile)) {
                    dataFileOut = "STDOUT";
                }
                utils::StdInOutHandler io_handler(dataFile, dataFileOut);
                if (type) {
                    if (type.Get() == utils::convertors::FileType::bin) {
                        TCPClientApp::sendBinFile(connectionInfo, dataFile, dataFileOut);
                    } else if (type.Get() == utils::convertors::FileType::hex) {
                        TCPClientApp::sendHexFile(connectionInfo, dataFile, dataFileOut);
                    } else {
                        TCPClientApp::sendDS8File(connectionInfo, dataFile, dataFileOut);
                    }
                } else {
                    TCPClientApp::sendDS8File(connectionInfo, dataFile);
                }
            } else {
                TCPClient client;
                if (!client.initConnection(connectionInfo)) {
                    std::cerr << "Init error\n";
                    exit(0);
                }
                if (!client.connect()) {
                    exit(0);
                }
                if (term) { // to send exit command to tcp_io component
                    client.send_terminate_exit();
                }
            }
        } // if (cmd_send)
        else if (cmd_convert) {

            if (itype.Get() == otype.Get()) {
                std::cerr << "Can not converting file it same type." << std::endl;
                error = 3;
                break;
            }

            std::string inFile(ifile.Get());
            std::string outFile(ofile.Get());
            utils::StdInOutHandler io_handler(inFile, outFile);
            switch (itype.Get()) {
            case utils::convertors::FileType::bin: {
                switch (otype.Get()) {
                case utils::convertors::FileType::hex:
                    utils::convertors::file_bin_to_hex(inFile, outFile, tag);
                    break;
                case utils::convertors::FileType::ds8:
                    utils::convertors::file_bin_to_ds8(inFile, outFile, blockSize.Get());
                    break;
                default:
                    std::cerr << "Unsupport file type" << std::endl;
                }
                break;
            }
            case utils::convertors::FileType::hex: {
                switch (otype.Get()) {
                case utils::convertors::FileType::bin:
                    utils::convertors::file_hex_to_bin(inFile, outFile);
                    break;
                case utils::convertors::FileType::ds8:
                    utils::convertors::file_hex_to_ds8(inFile, outFile, blockSize.Get());
                    break;
                default:
                    std::cerr << "Unsupport file type" << std::endl;
                }
                break;
            }
            case utils::convertors::FileType::ds8: {
                switch (otype.Get()) {
                case utils::convertors::FileType::bin:
                    utils::convertors::file_ds8_to_bin(inFile, outFile);
                    break;
                case utils::convertors::FileType::hex:
                    utils::convertors::file_ds8_to_hex(inFile, outFile, tag);
                    break;
                default:
                    std::cerr << "Unsupport file type" << std::endl;
                }
                break;
            }
            }
        } // if (cmd_convert)

    } while (false);

    return error;
}
