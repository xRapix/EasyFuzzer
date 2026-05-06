#include <iostream>
#include <string>
#include <curl/curl.h>
#include <fstream>
#include <thread> 
#include <vector>
#include <mutex>
#include <atomic>
#include <queue>
#include <condition_variable>

using namespace std;

size_t silent(void* p, size_t s, size_t n, void* d) { return s * n; }


int main(){

    string url;
    string wordlistPath;
    string User_Agent;
    atomic<int> code200found = 0;
    atomic<int> code404found = 0;
    atomic<int> code403 = 0;
    atomic<int> otherCodes = 0;

    queue<string> url_queue;

    cout<<"URL: ";
    cin >> url;
    cout << "WORDLIST: ";
    cin >> wordlistPath;
    cin.clear(); 
    fflush(stdin);
    cout << "USER-AGENT(defualt: curl/7.68.0)";
    getline(cin, User_Agent);

    if (User_Agent.empty()){
        User_Agent = "curl/7.68.0";
    }
    mutex mtx;

    string line;
    ifstream plik(wordlistPath);
    struct curl_slist *header = NULL;
    string full_au = "User-Agent: " + User_Agent;
    header = curl_slist_append(header, full_au.c_str());
    vector<thread> workers;
    atomic <int> active_threads(0);



    if (plik.is_open()){
        while (getline(plik,line)){
            active_threads++;
            workers.push_back(thread([&, line]() {
                CURL *curl = curl_easy_init();
                curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
                curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
                curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 2L); 
                curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

                if (curl){
                    string NewUrl = url + "/" + line;
                    curl_easy_setopt(curl,CURLOPT_URL,NewUrl.c_str());
                    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, silent);
                    CURLcode res = curl_easy_perform(curl);
                    string colorCode = "";

                    if (res == CURLE_OK){
                        long response_code;
                        curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE, &response_code);
                        if (response_code == 200){
                            colorCode = "\033[32m";
                            code200found++;
    
                        }
                        else if (response_code == 404){
                            colorCode = "\033[31m";
                            code404found++;
                        

                        }
                        else if (response_code == 403 or response_code == 400){
                            colorCode = "\033[33m";
                            code403++;
                        }
                        else if (response_code == 301 or response_code == 302){
                            colorCode = "\033[34m";
                            otherCodes++;
                        }
                        mtx.lock();
                        cout <<"URL: " << NewUrl << colorCode << " [" << response_code <<  "] " << "\033[0m" << endl;
                        mtx.unlock();
                    }
                    else{
                        mtx.lock();
                        cout << "\033[31m" << "URL: " << NewUrl << " " << curl_easy_strerror(res) << "\033[0m" << endl;
                        mtx.unlock();
                    }
                curl_easy_cleanup(curl);
                active_threads--;
                }
            }));
    while (active_threads >= 20) {
        this_thread::sleep_for(chrono::milliseconds(1)); 
    }
        }
    }    
    plik.close();
    for (auto &t : workers) t.join();
    cout << "---------STATS---------"<< endl << "200 Codes: " << code200found << endl << "404 Codes: " << code404found << endl << "403/400 Codes: " << code403 << endl << "Other codes: " << otherCodes << endl << "-----------------------";
    return 0;
}
