#include "crow.h"
#include <iostream>
#include <array>
#include <memory>
#include <stdexcept>
#include <regex>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>


//Вспомогательные функции

//несейвовый резолвинг хоста
std::string execCommand(const std::string& cmd) 
{
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    
    if (!pipe) 
    {
        throw std::runtime_error("popen() failed!");
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) 
    {
        result += buffer.data();
    }
    
    return result;
}

//сейвовый резолвинг хоста 
std::string execCommandSafe(const std::string& cmd)
{
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        throw std::runtime_error("error pipe()");
    }
    pid_t pid = fork();
    if (pid == -1)
    {
        throw std::runtime_error("error fork()");
    }
    if (pid == 0)
    {//родительский
        close(pipefd[0]); 
        dup2(pipefd[1], STDOUT_FILENO);
        dup2(pipefd[1], STDERR_FILENO);
        close(pipefd[1]); 

        char* const argv[] = 
        {
            const_cast<char*>("ping"),
            const_cast<char*>("-c"),
            const_cast<char*>("3"),
            const_cast<char*>(cmd.c_str()),
            nullptr
        };

        execvp("ping", argv);

        exit(EXIT_FAILURE);
    }
    else
    { //дочерний
        close(pipefd[1]);
        
        std::string result;
        char buffer[128];
        ssize_t count;
        
        while ((count = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
            result.append(buffer, count);
        }
        close(pipefd[0]);

        int status;
        waitpid(pid, &status, 0);

        return result; 
    }
}
bool isValidHost(const std::string& host) 
{
    std::regex hostRegex("^[a-zA-Z0-9.-]+$");
    return std::regex_match(host, hostRegex);
}

int main(int argc, char* argv[]) 
{
    crow::SimpleApp app;

    // Уязвимый эндпоинт- OS Command Injection
    CROW_ROUTE(app, "/api/ping").methods("POST"_method)([](const crow::request& req) {
        auto jsonPtr = crow::json::load(req.body);
        
        if (!jsonPtr || !jsonPtr.has("host")) {
            crow::json::wvalue errorJson;
            errorJson["error"] = "no host provided";
            return crow::response(400, errorJson);
        }
        
        std::string host = jsonPtr["host"].s();
        
        try {
            // host напрямую встраивается в команду (уязвимость)
            std::string command = "ping -c 3 " + host;
            std::string output = execCommand(command);
            
            crow::json::wvalue result;
            result["status"] = "success";
            result["output"] = output;
            return crow::response(200, result);
            
        } catch (const std::exception& e) {
            crow::json::wvalue errorJson;
            errorJson["error"] = e.what();
            return crow::response(500, errorJson);
        }
    });

    //исправленный эндпоинт
    CROW_ROUTE(app, "/api/ping_safe").methods("POST"_method)([](const crow::request& req) {
        auto jsonPtr = crow::json::load(req.body);
        
        if (!jsonPtr || !jsonPtr.has("host")) {
            crow::json::wvalue errorJson;
            errorJson["error"] = "no host provided";
            return crow::response(400, errorJson);
        }
        
        std::string host = jsonPtr["host"].s();
        
        if (!isValidHost(host)) {
            crow::json::wvalue errorJson;
            errorJson["error"] = "incorrect host provided";
            return crow::response(400, errorJson);
        }
        
        try {
            // host формируется только из ip-адреса целевого хоста
            std::string command = host;
            std::string output = execCommandSafe(command);
            
            crow::json::wvalue result;
            result["status"] = "success";
            result["output"] = output;
            return crow::response(200, result);
            
        } catch (const std::exception& e) {
            crow::json::wvalue errorJson;
            errorJson["error"] = e.what();
            return crow::response(500, errorJson);
        }
    });

    std::cout << "server started" << std::endl;
    app.port(5000).multithreaded().run();
    
    return 0;
}