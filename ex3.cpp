// Dor Levy 313547085
// The program supports Configuration File Option 2
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <thread>
#include <unistd.h>
#include <pthread.h>
#include <mutex>
#include <queue>
#include <semaphore.h>
using namespace std;
class BoundedQueue;
class UnboundedQueue;
vector<BoundedQueue*> queueArray;
UnboundedQueue* s;
UnboundedQueue* n;
UnboundedQueue* w;
BoundedQueue* mixedQueue;

struct producerArgs {
    int producerIndex;
    int numOfNews;
    int queueSize;
};

class BoundedQueue : public queue<string> {
    mutex mtx;
    sem_t full;
    sem_t empty;
public:

    BoundedQueue(int size) {
        sem_init(&full, 0, 0);
        sem_init(&empty, 0, size);
    }

    void enqueue(string news) {
        sem_wait(&empty);
        mtx.lock();
        queue::push(news);
        mtx.unlock();
        sem_post(&full);
    }

    string dequeue() {
        sem_wait(&full);
        mtx.lock();
        string news = queue::front();
        queue::pop();
        mtx.unlock();
        sem_post(&empty);
        return news;
    }
};

class UnboundedQueue : public queue<string> {
    mutex mtx;
    sem_t full;
public:

    UnboundedQueue() {
        sem_init(&full, 0, 0);
    }

    void enqueue(string news) {
        mtx.lock();
        queue::push(news);
        mtx.unlock();
        sem_post(&full);
    }

    string dequeue() {
        sem_wait(&full);
        mtx.lock();
        string news = queue::front();
        queue::pop();
        mtx.unlock();
        return news;
    }
};

void producer(void* structOfArgs) {
    auto *args = (struct producerArgs *)structOfArgs;
    int sportsCounter = 0;
    int newsCounter = 0;
    int weatherCounter = 0;
    string news;
    srand(time(0));
    for (int j = 0; j < args->numOfNews; j++) {
        switch ((rand() % 3)) {
            case 0:
                sportsCounter++;
                news = "Producer " + to_string(args->producerIndex) + " SPORTS " + to_string(sportsCounter);
                queueArray[args->producerIndex - 1]->enqueue(news);
                break;
            case 1:
                newsCounter++;
                news = "Producer " + to_string(args->producerIndex) + " NEWS " + to_string(newsCounter);
                queueArray[args->producerIndex - 1]->enqueue(news);
                break;
            case 2:
                weatherCounter++;
                news = "Producer " + to_string(args->producerIndex) + " WEATHER " + to_string(weatherCounter);
                queueArray[args->producerIndex - 1]->enqueue(news);
        }
    }
    news = to_string(-1);
    queueArray[args->producerIndex - 1]->enqueue(news);
}

void dispatcher(void* numOfProducersArg) {
    int numOfProducers = *(int *)numOfProducersArg;
    s = new UnboundedQueue();
    n = new UnboundedQueue();
    w = new UnboundedQueue();
    string sportString = "SPORTS";
    string weatherString = "WEATHER";
    string newsString = "NEWS";
    int doneCounter = 0;
    bool producerQueueHasFinished[numOfProducers];
    string news;
    for (int i = 0; i < numOfProducers; i++) {
        producerQueueHasFinished[i] = false;
    }
    while(1) {
        for (int i = 0; i < numOfProducers; i++) {
            if (doneCounter == numOfProducers) {
                break;
            }
            if (!producerQueueHasFinished[i]) {
                news = queueArray[i]->dequeue();
            } else {
                continue;
            }
            if (news == to_string(-1)) {
                producerQueueHasFinished[i] = true;
                doneCounter++;
                continue;
            }
            if (news.find(sportString) != string::npos) {
                s->enqueue(news);
            }
            else if (news.find(weatherString) != string::npos) {
                w->enqueue(news);
            }
            else {
                n->enqueue(news);
            }
        }
        if (doneCounter == numOfProducers) {
            s->enqueue(to_string(-1));
            n->enqueue(to_string(-1));
            w->enqueue(to_string(-1));
            break;
        }
    }
}

void editor(void* indexArg) {
    int editorIndex = *(int *)indexArg;
    string news;
    while(1) {
        switch (editorIndex) {
            case 0:
                news = s->dequeue();
                if (news == to_string(-1)) {
                    mixedQueue->enqueue(news);
                    return;
                }
                usleep(100000);
                mixedQueue->enqueue(news);
                break;
            case 1:
                news = n->dequeue();
                if (news == to_string(-1)) {
                    mixedQueue->enqueue(news);
                    return;
                }
                usleep(100000);
                mixedQueue->enqueue(news);
                break;
            case 2:
                news = w->dequeue();
                if (news == to_string(-1)) {
                    mixedQueue->enqueue(news);
                    return;
                }
                usleep(100000);
                mixedQueue->enqueue(news);
        }
    }
}

void screenManager() {
    int doneCounter = 0;
    string news;
    while (1) {
        if(doneCounter == 3) {
            cout << "DONE\n";
            return;
        }
        news = mixedQueue->dequeue();
        if(news == to_string(-1)) {
            doneCounter++;
            continue;
        }
        cout << news + "\n";
    }
}

int main(int argc, char *argv[])
{
    string line;
    vector<int> conFileLines;
    ifstream MyReadFile(argv[1]);
    while (getline(MyReadFile, line)) {
        if(line == "\r") {
            continue;
        }
        conFileLines.push_back(stoi(line));
    }
    MyReadFile.close();
    unsigned numOfProducers = (conFileLines.size() - 1) / 3;
    int editorQueueSize = conFileLines[conFileLines.size() - 1];
    producerArgs producerArgsArray[numOfProducers];
    int j = 0;
    for(int i = 0; i < numOfProducers; i++) {
        producerArgsArray[i].producerIndex = conFileLines[j];
        producerArgsArray[i].numOfNews = conFileLines[j + 1];
        producerArgsArray[i].queueSize = conFileLines[j + 2];
        j += 3;
    }
    BoundedQueue* bq;
    pthread_t producerThreadsArray[numOfProducers];
    for (int i = 0; i < numOfProducers; i++) {
        //BoundedQueue* bq;
        bq = new BoundedQueue(producerArgsArray[i].queueSize);
        queueArray.push_back(bq);
        pthread_create(&producerThreadsArray[i], nullptr, reinterpret_cast<void *(*)(void *)>(producer),
                       (void *)&producerArgsArray[i]);
    }
    usleep(50000);
    pthread_t dispatcherThread;
    pthread_create(&dispatcherThread, nullptr, reinterpret_cast<void *(*)(void *)>(dispatcher),
                   (void *)&numOfProducers);
    mixedQueue = new BoundedQueue(editorQueueSize);
    usleep(50000);
    pthread_t editorThreadsArray[3];
    int editorsIndex[3] = {0 ,1, 2};
    for (int i = 0; i < 3; i++) {
        pthread_create(&editorThreadsArray[i], nullptr, reinterpret_cast<void *(*)(void *)>(editor),
                       (void *)(void*)&editorsIndex[i]);
    }
    pthread_t screenManagerThread;
    pthread_create(&screenManagerThread, nullptr, reinterpret_cast<void *(*)(void *)>(screenManager), nullptr);
    pthread_join(screenManagerThread, nullptr);
    return 0;
}





