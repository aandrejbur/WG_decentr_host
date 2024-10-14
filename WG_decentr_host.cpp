#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include "json.hpp"
#include "getport.hpp"
#include <filesystem>
#include <algorithm>
#include <windows.h>
#include <tchar.h>
#include "StdCapture.h"
#include "windows.h"
#include "ShellAPI.h"
#include <chrono>
using namespace std::chrono;
using std::string;
namespace fs = std::filesystem;
using json = nlohmann::json;

//pathes
#define STATUS_PATH      "c:\\TomiVPN\\TomiVpn_status.json"
#define CONFIG_PATH      "c:\\TomiVPN\\wg98.conf"
#define WG_HOST_PATH     "c:\\TomiVPN\\TomiVpn_host.exe"
#define LOG_PATH         "c:\\TomiVPN\\TomiVpn_log.txt"
#define WIRE_GUARD_PATH  "c:\\TomiVPN\\"

//responses
#define UNDEFINED_ERROR_RESPONSE    "undefined_error"
#define INSTALLED_RESPONSE          "installed"
#define UNINSTALLED_RESPONSE        "uninstalled"
#define EXITED_RESPONSE             "exited"

//requests
#define WG_SHOW                     "is_wgTunnel_installed"
#define UNINSTALL_TUNNEL_COMMAND    "uninstall_wg_tunnel"
#define INSTALL_TUNNEL_COMMAND      "install_wg_tunnel"

//communicative files pathes
const fs::path response_file     {"c:\\TomiVPN\\response.rspn"};
const fs::path request_file_path {"c:\\TomiVPN\\request.rqst"};


bool disconnectActiveConnection();
bool connectToVPN();
bool isWG_installed();
bool isVPN_TunnelInstalled();

void writeRequestFile(const string &request);
string waitForResponse();



int main()
{
 
	
	// length of the input and output message
    unsigned long inLength = 0, outLength = 0;

    // in/out message
    string inMessage, outMessage;
    std::stringstream outMessage_ss;
    json message;

    // a bit of logging
    //std::ofstream logFile;
    //logFile.open("MessagingLog.txt", std::ios::app);

    // read the first four bytes (length of input message
    for (int index = 0; index < 4; index++) { unsigned int read_char = getchar(); inLength = inLength | (read_char << index * 8); }

   // logFile << "Length: " << inLength << std::endl;

    // read the message form the extension
    for (int index = 0; index < (int)inLength; index++) { inMessage += getchar(); }

    
    //inMessage = R"({"type":"connect","params": {"ipV4":"10.8.0.3","ipV6":"fd86:ea04:1115:0000:0000:0000:0000:0003","host":"170.187.141.223","port":61409,"hostPublicKey":"BS9Iuy+1LH/Z9uZXUD9FUzb8P9TFnZ4IIfWKxoMMM08=","wgPrivateKey":"UG7PflH9H0OLnrGdprx2WwQ0/YiFJRKe7oRaOivK6l0=","address":"sent1tet7xxem50t6hxfh605ge3r30mau7gl9kd820n","sessionId":55680,"nodeAddress":"sentnode1yfwfsky2usqudsnx7t6xhx4xqsz79zu2va8fws"}})";
    //inMessage = R"({"type":"status"})";
    //inMessage = R"({"type":"disconnect"})";
    //inMessage = R"({"type":"isWgInstalled"})";
    //inMessage = R"({"type":"wgInstall"})";
	

    //logFile << "Received: " << inMessage << std::endl;  
    inMessage.erase(std::remove(inMessage.begin(), inMessage.end(), '\\'), inMessage.end());

	
	
    
    try
    {
        //logFile << "Is valid json: " << std::boolalpha << json::accept(inMessage) << std::endl;
        message = json::parse(inMessage);
        //logFile << "JSON: " << message.dump() << std::endl;

        if (message.find("type") == message.end()){

            outMessage_ss << "{\"error\":\"invalid json\"}";
        }
        else {

                 if (message["type"].get<string>() == "connect"){
                
                   // check that json has required parameters              
                if (message.find("params") == message.end()){

                    outMessage_ss << "{\"error\":\"invalid json, no 'params'\"}";
                }
                else{
                    
                    uint16_t port =  GetFreeUDPPort();
                    //logFile << "Port: " << port << std::endl;
                    
                    json params = message["params"];
                    // create config file
                    std::ofstream configFile;
                    configFile.open(CONFIG_PATH);
                    configFile << "[Interface]" << std::endl;
                    configFile << "PrivateKey = " << params["wgPrivateKey"].get<string>() << std::endl;
                    configFile << "ListenPort = " << port << std::endl;
                    
                    configFile << "Address = " << params["ipV4"].get<std::string>() << "/32";

                    if (params.contains("ipV6"))
                    {
                        configFile << ", " << params["ipV6"].get<std::string>() << "/128" << std::endl;
                    }
                    else {
                        configFile << std::endl;
                    }
                    if (params.contains("DNS"))
                    {
                        configFile << "DNS = " << params["DNS"].get<std::string>() << std::endl;
                    }
                    else {
                        configFile << "DNS = 10.8.0.1" << std::endl;
                    }

                    configFile << "[Peer]" << std::endl;
                    configFile << "PublicKey = " << params["hostPublicKey"].get<string>() << std::endl;
                    configFile << "AllowedIPs = 0.0.0.0/0, ::/0" << std::endl;
                    configFile << "Endpoint = " << params["host"].get<string>() << ":" << params["port"].get<int>() << std::endl;
                    configFile << "PersistentKeepalive = 15" << std::endl;
                    configFile.close();
                   
                    
                    // first uninstall tunnel service if it is installed
                    disconnectActiveConnection();                   

                    if (connectToVPN()) {

                        // create status file
                        json status_js;
                        status_js["address"] = params["address"].get<string>();
                        status_js["sessionId"] = params["sessionId"].get<int>();
                        status_js["interface"] = "wg98";
                        status_js["nodeAddress"] = params["nodeAddress"].get<string>();
                        std::ofstream statusFile;
                        statusFile.open(STATUS_PATH);
                        statusFile << status_js.dump();
                        statusFile.close();
                        outMessage_ss << "{\"result\":true" << ",\"response\":" << status_js.dump() << "}";
                                           
                    }
                    else {

                        outMessage_ss << "{\"result\":false}";

                    }
                }
            }
            else if (message["type"].get<string>() == "status")
            {
                
                if (isVPN_TunnelInstalled())
                {
                                                       
                    if (fs::exists(STATUS_PATH))
                    {
                        std::ifstream statusFile(STATUS_PATH);
                        string line;
                        if (statusFile.is_open())
                        {
                            std::getline(statusFile, line);
                        }
                        outMessage_ss << "{\"result\":true, \"response\":" + line + "}";
                    }
                    else {
                        outMessage_ss << "{\"result\":true}";
                    }
                                      
                }
                else
                {
                    outMessage_ss << "{\"result\":false}";
                }
            }
            else if (message["type"].get<string>() == "disconnect")
            {
                
                if (disconnectActiveConnection())
                {
                    fs::remove(STATUS_PATH);
                    outMessage_ss << "{\"result\":true}";                  
                }
                else {

                    outMessage_ss << "{\"result\":false}";

                }
            }
            else if (message["type"].get<string>() == "isWgInstalled") {
            
                if (isWG_installed())
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
        //logFile << "JSON exception: " << e.what() << ", exception id: " << e.id;
        outMessage_ss << "{\"error\":\"json exception\"}";

    }

    // collect the length of the message
    outLength = outMessage_ss.str().length();

    // send the 4 bytes of length information //
    std::cout.write(reinterpret_cast<const char*>(&outLength), 4);
    // send output message
    std::cout << outMessage_ss.str() << std::flush;

    // a bit of logging
    //logFile << "Sent: " << outMessage_ss.str() << std::endl;
	//logFile <<"******************************"<<std::endl<<std::endl;
   // logFile.close();

    return EXIT_SUCCESS;
}

bool disconnectActiveConnection() {
    
    string result = "";	
    writeRequestFile(UNINSTALL_TUNNEL_COMMAND);
    result = waitForResponse();
	if(result == UNINSTALLED_RESPONSE) return true;
	return false;

}

bool connectToVPN() {

    string result = "";
    writeRequestFile(INSTALL_TUNNEL_COMMAND);
	result = waitForResponse();
    if(result == INSTALLED_RESPONSE) return true;
    return false;
}


bool isWG_installed() {

    if (fs::exists(WIRE_GUARD_PATH))return true;
    return false;

}

bool isVPN_TunnelInstalled(){
   
	writeRequestFile(WG_SHOW);
	string response = waitForResponse();
    if (response == INSTALLED_RESPONSE)return true;
    return false;
}


void writeRequestFile(const string& request)
{
	std::ofstream requestFile(request_file_path, std::ios::trunc);
	if(requestFile.is_open()){
	requestFile << request;
	requestFile.close();
	}
}

string waitForResponse()
{
	auto start = high_resolution_clock::now();
	while(!fs::exists(response_file)){
	
		Sleep(20);
		auto stop = high_resolution_clock::now();
	    auto duration = duration_cast<seconds>(stop - start);
		if (duration.count() > 5) {//if coommunicator doesn't response ---> run it after 5 seconds 
			//Beep(500,500);
			ShellExecute(0, L"runas", (LPCWSTR)L"c:\\TomiVPN\\TomiVpn_com.exe",0, 0 , SW_SHOW);
			exit(0);
		};

	}
    
	string response = "";
	std::ifstream responseFile(response_file);
	if(responseFile.is_open()){
	std::getline(responseFile, response);
    responseFile.close();

	   try{

		fs::remove(response_file);

	   }catch(fs::filesystem_error e){
	      response = e.what();
	      //Beep(1000,1000);
	   }

	}
	
	return response;
}

