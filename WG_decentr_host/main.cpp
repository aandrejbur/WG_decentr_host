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


using json = nlohmann::json;

#define STATUS_PATH "/tmp/WG_status.json"
#define LOG_PATH "/tmp/WG_log.txt"
#define CONFIG_PATH "/tmp/wg98.conf"

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

int main()
{
    // length of the input and output message
    unsigned long inLength = 0, outLength = 0;
    // in/out message
    std::string inMessage, outMessage;
    std::stringstream outMessage_ss;
    json message;
    
    // a bit of logging
    std::ofstream logFile;
    logFile.open(LOG_PATH, std::ios::app);
    
    // read the first four bytes (length of input message
    for (int index = 0; index < 4; index++){unsigned int read_char = getchar();inLength = inLength | (read_char << index*8);}
    
    logFile << "Length: " << inLength << std::endl;

    //read the message form the extension
    for (int index = 0; index < inLength; index++){inMessage += getchar();}
    
    
    //inMessage = R"({"type":"connect","params": {"ipV4":"10.8.0.3","ipV6":"fd86:ea04:1115:0000:0000:0000:0000:0003","host":"170.187.141.223","port":61409,"hostPublicKey":"BS9Iuy+1LH/Z9uZXUD9FUzb8P9TFnZ4IIfWKxoMMM08=","wgPrivateKey":"UG7PflH9H0OLnrGdprx2WwQ0/YiFJRKe7oRaOivK6l0=","address":"sent1tet7xxem50t6hxfh605ge3r30mau7gl9kd820n","sessionId":55680,"nodeAddress":"sentnode1yfwfsky2usqudsnx7t6xhx4xqsz79zu2va8fws"}})";
    
    //inMessage = R"({"type":"status"})";
    
    //inMessage = R"({"type":"disconnect"})";
    
    //inMessage = R"({"type":"isWgInstalled"})";
    
    logFile << "Received: " << inMessage << std::endl;
    
    inMessage.erase(std::remove(inMessage.begin(), inMessage.end(), '\\'), inMessage.end());
    
    
    try
    {
        logFile << "Is valid json: " << std::boolalpha <<  json::accept(inMessage) << std::endl;
        message = json::parse(inMessage);
        logFile <<"JSON: " << message.dump() << std::endl;
        
        if(message.find("type")==message.end())
        {
            outMessage_ss << "{\"error\":\"invalid json\"}";
        }else{
            setenv("PATH", "/Applications/Decentr:/opt/homebrew/bin:/opt/homebrew/sbin:/usr/local/bin:/usr/bin:/bin:/usr/sbin:/sbin", 1);
            
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
                    
                    uint16_t port = 52487;// GetFreeUDPPort();
                    logFile << "Port: " << port << std::endl;
                    
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
                    command_ss_up << "/usr/bin/osascript -e 'do shell script \" bash -c \\\"wg-quick down "<< CONFIG_PATH;
                    command_ss_up <<" \\\"; bash -c \\\"wg-quick up "<< CONFIG_PATH ;
                    command_ss_up << " \\\"; echo worked \" with administrator privileges with prompt \"Enter password to connect Decentr VPN\"' ";
                    
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
                    }
                    
                }
            }
            else if(message["type"].get<std::string>()=="status")
            {
                //check also wireguard-go
                    
                    std::string command = "ps -A | grep wireguard-go | grep -v grep";
                    
                    
                    if(exec(command.c_str()).empty())
                    {
                    
                        outMessage_ss << "{\"result\":false}";
                        
                        std::string command_2 ="osascript -e 'display alert \"Decentr VPN is disconnected\"'";
                        exec(command_2.c_str());
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
                std::stringstream command_ss_down;
                command_ss_down << "/usr/bin/osascript -e 'do shell script \"bash -c \\\"wg-quick down "<< CONFIG_PATH <<"\\\" ; echo worked \" with administrator privileges with prompt \"Enter password to disconnect Decentr VPN\"' ";
                
                if(exec(command_ss_down.str().c_str()).empty())
                {
                    outMessage_ss << "{\"result\":false}";
                }else{
                    std::filesystem::remove(STATUS_PATH);
                    outMessage_ss << "{\"result\":true}";
                }
            }
            else if(message["type"].get<std::string>()=="isWgInstalled")
            {
                outMessage_ss << "{\"result\":true}";
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
        logFile << "JSON exception: "<< e.what() << ", exception id: "<< e.id;
        outMessage_ss <<"{\"error\":\"json exception\"}";
        
    }
        
    // collect the length of the message
    outLength = outMessage_ss.str().length();
    
    // send the 4 bytes of length information //
    std::cout.write(reinterpret_cast<const char *>(&outLength), 4);
    // send output message
    std::cout << outMessage_ss.str() << std::flush;

    // a bit of logging
    logFile << "Sent: " << outMessage_ss.str() << std::endl;
    logFile.close();
    
    return 0;
}
