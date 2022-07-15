#include <iostream>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <string>
#include <fstream>
#include <unordered_map>
#include <thread>
#include <vector>
#include <filesystem>

using namespace std::string_literals;
enum ERROR{
    CANT_OPEN_FILE = 1,
    CANT_READ_FROM_FILE = 2,
    CANT_CLOSE_FILE = 3,
    WRONG_ARGUMENT_NUMBER = 4,
    CANT_OPEN_CONFIG = 5
};
struct InputData{
    int indexing_threads = 1;
    std::string indir;
    std::string out_by_a;
    std::string out_by_n;
};
//#define DEBUG

inline std::chrono::high_resolution_clock::time_point get_current_time_fenced() {
    std::atomic_thread_fence(std::memory_order_seq_cst);
    auto res_time = std::chrono::high_resolution_clock::now();
    std::atomic_thread_fence(std::memory_order_seq_cst);
    return res_time;
}

template <class D>
int to_us(const D &d) {
#ifdef DEBUG
    std::cout << std::chrono::duration_cast<std::chrono::microseconds>(d).count() << std::endl;
#endif
    return std::chrono::duration_cast<std::chrono::microseconds>(d).count();
}

template <typename T>
class MultiThreadQueue{
public:
    MultiThreadQueue() = default;
    ~MultiThreadQueue() = default;
    MultiThreadQueue(const MultiThreadQueue&) = delete;
    MultiThreadQueue& operator = (const MultiThreadQueue&) = delete;

    // add value into queue
    void enque(const T& a) {
        {
            std::unique_lock<std::mutex> lock{mutexM};
            condVarM.wait(lock, [this]() { return queueMT.size() < MAX_SIZE; });
            queueMT.push_back(a);
        }
        condVarM.notify_one();
    }

    void enque(T&& a) {
        {
            std::unique_lock<std::mutex> lock(mutexM);
            condVarM.wait(lock, [this]() { return queueMT.size() < MAX_SIZE; });
            queueMT.push_back(std::move(a));
        }
        condVarM.notify_one();
    }

    // pop one value from queue
    T deque(){
        std::unique_lock<std::mutex> lock(mutexM);
        while (queueMT.empty()) {
            condVarM.wait(lock);
        }
        auto a = queueMT.front();
        queueMT.pop_front();
        return a;
    }

    size_t getSize() const {
        std::lock_guard<std::mutex> lock(mutexM);
        return queueMT.size();
    }

private:

    std::deque<T> queueMT;
    mutable std::mutex mutexM;
    std::condition_variable condVarM;
    constexpr const static size_t MAX_SIZE = 10000;
};

InputData dataParser(){

    InputData inpData;
    std::ifstream configurator("configuration.cfg");


    if (configurator.is_open()) {
        std::string parameter;
        while (getline(configurator, parameter)) {
            parameter.erase(std::remove_if(parameter.begin(),parameter.end(),
                                           isspace),parameter.end());

            if (parameter.empty() or parameter[0] == '\0') {
                continue;
            }

            size_t equalityChar = parameter.find('=');
            std::string param = parameter.substr(0, equalityChar);
            std::string paramValue = parameter.substr(equalityChar + 1);

            if (param == "out_by_n") {
                inpData.out_by_n = paramValue;
            }
            else if (param == "indexing_threads") {
                inpData.indexing_threads = std::stoi(paramValue);
            }
            else if (param == "out_by_a") {
                inpData.out_by_a = paramValue;
            }
            else if (param == "indir") {
                inpData.indir = paramValue;
            }

        }
        configurator.close();
    }
    else {
        std::cerr << "Couldn't open config file for reading.\n";
        exit(CANT_OPEN_CONFIG);
    }

    return inpData;
}

void readFiles(const std::string &path, MultiThreadQueue<std::filesystem::path> &paths) {
    using recursive_directory_iterator = std::filesystem::recursive_directory_iterator;
    for (const auto& dirEntry : recursive_directory_iterator("data"))
        paths.enque(dirEntry.path());

    paths.enque(std::filesystem::path(""s));
}

void readDataFromFiles(MultiThreadQueue<std::filesystem::path> *qFirst, MultiThreadQueue<std::string>* qSecond){
    while (true){
        auto fileName = qFirst->deque();
        std::string strData;
#ifdef DEBUG
        std::cout << fileName + '\n'<< std::endl;
#endif
        if (fileName == ""s){
            qSecond->enque(""s);

#ifdef DEBUG
        std::cout << "read data killed\n";
#endif
            break;
        }
        std::getline(std::ifstream(fileName), strData, '\0');
        qSecond->enque(strData);
    }
}

std::unordered_map<std::string, int> globalDictionary;
void indexingFunction(MultiThreadQueue<std::string> &qSecond,
                     MultiThreadQueue<std::unordered_map<std::string, int>> &qThird,
                     std::mutex &mutexIndexing) {
    while (true) {
        std::unordered_map<std::string, int> dict;

        auto file = qSecond.deque();
        auto sizeStr = file.size() + 1;

        if (file == ""s) {
#ifdef DEBUG
            std::cout << "KILLED" << std::endl;
#endif
            qSecond.enque(""s);
            break;
        }

        size_t i = 0;
        std::string letter;
        while (i != sizeStr) {
            if (isspace(file[i]) || file[i] == '\0') {
                dict[letter] += 1;
                letter.clear();
            } else {
                letter += file[i];
            }
            i++;
        }

        mutexIndexing.lock();
        globalDictionary.insert(dict.begin(), dict.end());
        mutexIndexing.unlock();

    }

}

void writeResultsToFiles(std::unordered_map<std::string, int> dict,
                         std::string filename1, std::string filename2){
    std::vector<std::pair<std::string, int>> vect(dict.begin(), dict.end());
    std::sort(vect.begin(), vect.end());
    std::ofstream MyFile1(filename1);

    for (const auto &item : vect) {
        MyFile1 << item.first << " -- " << item.second << "\n";
    }
    MyFile1.close();

    std::sort(vect.begin(), vect.end(), [] (const auto &x, const auto &y) { return x.second > y.second; });
    std::ofstream MyFile2(filename2);
    for (const auto &item : vect) {
        MyFile2 << item.first << " -- " << item.second << "\n";
    }

    MyFile2.close();
}


int main() {
    auto timeStartTOTAL = get_current_time_fenced();
    InputData inpdat;
    inpdat = dataParser();
    MultiThreadQueue<std::filesystem::path> paths;
    MultiThreadQueue<std::string> mtq2;
    MultiThreadQueue<std::unordered_map<std::string, int>> mtq3;
    std::vector<std::thread> ThreadVector;

    auto timeStartFIDNING = get_current_time_fenced();
    readFiles("data", paths);
    auto timeFinishFIDNING = get_current_time_fenced();

    auto timeStartREADING = get_current_time_fenced();
    std::thread readDataThread(readDataFromFiles, &paths, &mtq2);
    auto timeFinishREADING = get_current_time_fenced();



    std::mutex mutexIndexing;
    for (int i = 0; i < inpdat.indexing_threads; ++i) {
        ThreadVector.emplace_back(indexingFunction, std::ref(mtq2), std::ref(mtq3), std::ref(mutexIndexing));
    }

    readDataThread.join();
    for (int i = 0; i < inpdat.indexing_threads; ++i) {
        ThreadVector[i].join();
    }

    auto timeStartWRITING = get_current_time_fenced();
    writeResultsToFiles(globalDictionary, inpdat.out_by_a, inpdat.out_by_n);
    auto timeFinishWRITING = get_current_time_fenced();

    auto timeFinishTOTAL = get_current_time_fenced();

    std::cout << "Total=" << to_us(timeFinishTOTAL - timeStartTOTAL) << std::endl;
    std::cout << "Reading=" << to_us(timeFinishREADING - timeStartREADING) << std::endl;
    std::cout << "Finding=" << to_us(timeFinishFIDNING - timeStartFIDNING) << std::endl;
    std::cout << "Writing=" << to_us(timeFinishWRITING - timeStartWRITING) << std::endl;

    return 0;

}
//TODO: Add Python script
// TODO: error handle
// TODO: handle utf-8
