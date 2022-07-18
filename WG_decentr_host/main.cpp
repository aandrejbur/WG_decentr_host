#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include "json.hpp"
#include "getport.hpp"
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <thread>
#include <atomic>
#include <fcntl.h>
#include "errno.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <unistd.h>

using json = nlohmann::json;

#define STATUS_PATH "/tmp/WG_status.json"
//#define LOG_PATH "/tmp/WG_log.txt"
//#define LOG_PATH_CH "/tmp/WG_log_CH.txt"
#define CONFIG_PATH "/tmp/wg98.conf"
#define FIFO_PATH "/tmp/WG_fifo"

std::string exec(const char* cmd) {
    std::array<char, 1024> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

#define START 1
#define STOP  2
#define PAUSE 3

int makeChild()
{
    if(std::filesystem::exists(FIFO_PATH))
    {
      //nothing to do child already exists
        return 0;
    }else{
        //create child:
        pid_t pid;
        pid = fork();
        if(pid > 0 )
        {// parent
            return 0;
        }
        else
        {
            //printf("IM CHILD!!!!\n");
            // create file
            mkfifo(FIFO_PATH, S_IRWXU);
            //std::ofstream logFile;
            //logFile.open(LOG_PATH_CH, std::ios::app);
            //logFile << "Child started: " << std::endl;
            setenv("PATH", "/Applications/Decentr.app/Contents/Frameworks/Decentr Framework.framework/Helpers:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin", 1);
            int signal = 0;
            bool stop = false, check = false;
            int pipefd = open(FIFO_PATH,O_RDONLY | O_NONBLOCK);
            //logFile << "Pipe opened: " << std::endl;
            if(pipefd == -1){
                if(errno != EAGAIN)
                {
                    //logFile << "opening error not EAGAIN " << std::endl;
                    //logFile.close();
                    close(pipefd);
                    std::filesystem::remove(FIFO_PATH);
                    return 1;
                }
            }

            //logFile << "before cycle " << std::endl;
            while (stop == false)
            {
                
                size_t result = read(pipefd, &signal, sizeof(int));
                if( result == -1)
                {
                    if(errno != EAGAIN)
                    {
                        //logFile << "error not EAGAIN " << std::endl;
                        break;
                    }
                    //logFile << "EAGAIN " << std::endl;
                    continue;
                    // everything is ok, we just do not get any information
                }else{
                    //logFile << "Signal = " << signal<< std::endl;
                    switch (signal) {
                        case START:
                            check = true;
                            break;
                        case STOP:
                            stop = true;
                        case PAUSE:
                            check = false;
                        default:
                            break;
                    }
                    signal = 0;
                }
            
                if(stop == true)
                {
                    //logFile << "STOP "<<std::endl;
                    break;
                }
                else if(check == true)
                {
                    //logFile << "CHECK "<<std::endl;
                    std::string command = "ps -A | grep wireguard-go | grep -v grep";
                    if(exec(command.c_str()).empty())
                    {
                        //logFile << "CHECK failed, vpn down"<<std::endl;
                        // interface is down, notify user, delete status file, stop the execution
                        if(std::filesystem::exists(STATUS_PATH))
                        {
                            
                            //logFile << "before message"<<std::endl;
                            std::stringstream command_ss_reconect;
                            command_ss_reconect << "/usr/bin/osascript -e 'do shell script \"bash -c \\\"wg-quick down "<< CONFIG_PATH<<" \\\"; bash -c \\\"wg-quick up "<< CONFIG_PATH " \\\"; echo worked \" with administrator privileges with prompt \"Decentr VPN is disconnected!\n Enter password to reconect Decentr VPN\"' ";
                            if(exec(command_ss_reconect.str().c_str()).empty())
                            {
                                std::filesystem::remove(STATUS_PATH);
                                stop = true;
                                break;
                            }
                        }
                    }else{
                        //logFile << "Checks ok"<<std::endl;
                    }
                }
                //logFile << "BEfore sleep"<<std::endl;
                sleep(15);
            }
            
            //logFile << "Child closing files"<<std::endl;
            close(pipefd);
            //logFile.close();
            // remowe fifo file
            std::filesystem::remove(FIFO_PATH);
            return 1; // signal to exit
        }
    }
    return 0;
}

// sending byte to start checks
void SendSignal(int signal)
{
    int pipefd = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
    if(pipefd == -1){
        return;
    }
    write(pipefd, &signal, sizeof(int));
    close(pipefd);
}

//sending signal to stop checking

#define WG_PATH "/Applications/Decentr.app/Contents/Frameworks/Decentr Framework.framework/Helpers/wg"
#define WG_QUICK_PATH "/Applications/Decentr.app/Contents/Frameworks/Decentr Framework.framework/Helpers/wg-quick"
#define WG_GO_PATH "/Applications/Decentr.app/Contents/Frameworks/Decentr Framework.framework/Helpers/wireguard-go"
#define WG_BASH_PATH "/Applications/Decentr.app/Contents/Frameworks/Decentr Framework.framework/Helpers/bash"

bool isWGInstalled()
{
    if(std::filesystem::exists(WG_PATH) &&
       std::filesystem::exists(WG_QUICK_PATH) &&
       std::filesystem::exists(WG_GO_PATH) &&
       std::filesystem::exists(WG_BASH_PATH) )
    {
        return true;
    }
    
    return false;
}

int main()
{
    // length of the input and output message
    unsigned long inLength = 0, outLength = 0;
    // in/out message
    std::string inMessage, outMessage;
    std::stringstream outMessage_ss;
    json message;
    
    // a bit of logging
    //std::ofstream logFile;
    //logFile.open(LOG_PATH, std::ios::app);
    
    if(makeChild()>0)
        return 0;
    
    // read the first four bytes (length of input message
    for (int index = 0; index < 4; index++){unsigned int read_char = getchar();inLength = inLength | (read_char << index*8);}
    
    //logFile << "Length: " << inLength << std::endl;

    //read the message form the extension
    for (int index = 0; index < inLength; index++){inMessage += getchar();}
    
    
    //inMessage = R"({"type":"connect","params": {"ipV4":"10.8.0.3","ipV6":"fd86:ea04:1115:0000:0000:0000:0000:0003","host":"170.187.141.223","port":61409,"hostPublicKey":"BS9Iuy+1LH/Z9uZXUD9FUzb8P9TFnZ4IIfWKxoMMM08=","wgPrivateKey":"UG7PflH9H0OLnrGdprx2WwQ0/YiFJRKe7oRaOivK6l0=","address":"sent1tet7xxem50t6hxfh605ge3r30mau7gl9kd820n","sessionId":55680,"nodeAddress":"sentnode1yfwfsky2usqudsnx7t6xhx4xqsz79zu2va8fws"}})";
    
    //inMessage = R"({"type":"status"})";
    
    //inMessage = R"({"type":"disconnect"})";
    
    //inMessage = R"({"type":"isWgInstalled"})";
    
    //inMessage = R"({"type":"wgInstall"})";
    
    //logFile << "Received: " << inMessage << std::endl;
    
    inMessage.erase(std::remove(inMessage.begin(), inMessage.end(), '\\'), inMessage.end());
    
    
    
    try
    {
        //logFile << "Is valid json: " << std::boolalpha <<  json::accept(inMessage) << std::endl;
        message = json::parse(inMessage);
        //logFile <<"JSON: " << message.dump() << std::endl;
        
        if(message.find("type")==message.end())
        {
            outMessage_ss << "{\"error\":\"invalid json\"}";
        }else{
            setenv("PATH", "/Applications/Decentr.app/Contents/Frameworks/Decentr Framework.framework/Helpers:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin", 1);
            
            if (message["type"].get<std::string>()=="connect")
            {
                //Applications
                //check that json hsve required parameters
                if(message.find("params")==message.end())
                {
                    outMessage_ss << "{\"error\":\"invalid json, no 'params'\"}";
                }
                else
                {
                    
                    uint16_t port = GetFreeUDPPort();
                    //logFile << "Port: " << port << std::endl;
                    
                    json params = message["params"];
                    // create config file
                    std::ofstream configFile;
                    configFile.open(CONFIG_PATH);
                    configFile <<"[Interface]"<<std::endl;
                    configFile <<"PrivateKey = "<< params["wgPrivateKey"].get<std::string>() << std::endl;
                    configFile <<"ListenPort = "<< port << std::endl;
                    configFile <<"Address = " << params["ipV4"].get<std::string>() << "/32, "<< params["ipV6"].get<std::string>() << "/128"<< std::endl;
                    configFile <<"DNS = 10.8.0.1"<<std::endl;
                    configFile <<"[Peer]"<<std::endl;
                    configFile <<"PublicKey = "<< params["hostPublicKey"].get<std::string>() << std::endl;
                    configFile <<"AllowedIPs = 0.0.0.0/0, ::/0" << std::endl;
                    configFile <<"Endpoint = "<< params["host"].get<std::string>() <<":"<< params["port"].get<int>() <<  std::endl;
                    configFile <<"PersistentKeepalive = 15"<<std::endl;
                    configFile.close();
                    
                    std::stringstream command_ss_up;
                    // first disconnect wg if it is no connected
                    
                    command_ss_up << "/usr/bin/osascript -e 'do shell script \"bash -c \\\"wg-quick down "<< CONFIG_PATH<<" \\\" 2>&1; bash -c \\\"wg-quick up "<< CONFIG_PATH " \\\" 2>&1 \" with administrator privileges with prompt \"Enter password to connect Decentr VPN\"' ";
                    
                    if(exec(command_ss_up.str().c_str()).empty())
                    {
                        outMessage_ss << "{\"result\":false}";
                    }else{
                        // create status file
                        
                        json status_js;
                        status_js["address"] = params["address"].get<std::string>();
                        status_js["sessionId"] = params["sessionId"].get<int>();
                        status_js["interface"] = "wg98";
                        status_js["nodeAddress"] = params["nodeAddress"].get<std::string>();
                        std::ofstream statusFile;
                        statusFile.open(STATUS_PATH);
                        statusFile<<status_js.dump();
                        statusFile.close();
                        
                        outMessage_ss << "{\"result\":true" << ",\"response\":"<<status_js.dump()<<"}";
                        
                        SendSignal(START);
                    }
                    
                }
            }
            else if(message["type"].get<std::string>()=="status")
            {
                std::string command = "ps -A | grep wireguard-go | grep -v grep";
                if(exec(command.c_str()).empty())
                {
                    outMessage_ss << "{\"result\":false}";
                }
                else
                {
                    if(std::filesystem::exists(STATUS_PATH))
                    {
                        std::ifstream statusFile(STATUS_PATH);
                        std::string line;
                        if(statusFile.is_open())
                        {
                            std::getline(statusFile,line);
                        }
                        outMessage_ss << "{\"result\":true, \"response\":"+line+"}";
                    }else{
                        outMessage_ss << "{\"result\":true}";
                    }
                }
            }
            else if(message["type"].get<std::string>()=="disconnect")
            {
                SendSignal(PAUSE);
                std::stringstream command_ss_down;
                command_ss_down << "/usr/bin/osascript -e 'do shell script \"bash -c \\\"wg-quick down "<< CONFIG_PATH <<"\\\" ; echo worked \" with administrator privileges with prompt \"Enter password to disconnect Decentr VPN\"' ";
                
                if(exec(command_ss_down.str().c_str()).empty())
                {
                    outMessage_ss << "{\"result\":false}";
                    SendSignal(START);
                }else{
                    SendSignal(STOP);
                    std::filesystem::remove(STATUS_PATH);
                    outMessage_ss << "{\"result\":true}";
                }
            }
            else if(message["type"].get<std::string>()=="wgInstall")
            {
                if(isWGInstalled())
                {
                    outMessage_ss << "{\"result\":true}";
                }
                else
                {
                    outMessage_ss << "{\"result\":false}";
                }
            }
            else if(message["type"].get<std::string>()=="isWgInstalled")
            {
                if(isWGInstalled())
                {
                    outMessage_ss << "{\"result\":true}";
                }
                else
                {
                    outMessage_ss << "{\"result\":false}";
                }
            }
            else
            {
                outMessage_ss << "{\"error\":\"invalid type\"}";
            }
        }
        
    }
    catch (json::exception& e)
    {
        // output exception information
        //logFile << "JSON exception: "<< e.what() << ", exception id: "<< e.id;
        outMessage_ss <<"{\"error\":\"json exception\"}";
        
    }
        
    // collect the length of the message
    outLength = outMessage_ss.str().length();
    
    // send the 4 bytes of length information //
    std::cout.write(reinterpret_cast<const char *>(&outLength), 4);
    // send output message
    std::cout << outMessage_ss.str() << std::flush;

    // a bit of logging
    //logFile << "Sent: " << outMessage_ss.str() << std::endl;
    //logFile.close();
    
    return 0;
}
