#include"json.hpp"
#include<iostream>
#include<vector>
#include<map>
#

using namespace std;
using json = nlohmann::json;

string func1(){
    json js;
    js["msg_type"] = 2;
    js["from"] = "zhang san";
    js["to"] = "li si";
    js["masg"] = "hello";

    string sendBuf = js.dump();

    //cout << js << endl;
    return sendBuf;
}

int main(){
    
    string recvBuf = func1();
    json jsbuf = json::parse(recvBuf);
    cout << jsbuf["msg_type"] << endl;
    cout << jsbuf["from"] << endl;
    cout << jsbuf["to"] << endl;
    cout << jsbuf["msg"] << endl;
    return 0;

}