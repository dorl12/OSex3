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
    struct producerArgs *args = (struct producerArgs *)structOfArgs;
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

void dispatcher(int numOfProducers) {
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

void editor(int i) {
    string news;
    while(1) {
        switch (i) {
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

int main()
{
    string line;
    vector<string> conFileLines;
    ifstream MyReadFile("conFile.txt");
    while (getline(MyReadFile, line)) {
        if(line == "\r") {
            continue;
        }
        conFileLines.push_back(line);
    }
    MyReadFile.close();
    int numOfProducers = (conFileLines.size() - 1) / 3;
    int editorQueueSize = stoi(conFileLines[conFileLines.size() - 1]);
    producerArgs producerArgsArray[numOfProducers];
    int j = 0;
    for(int i = 0; i < numOfProducers; i++) {
        producerArgsArray[i].producerIndex = stoi(conFileLines[j]);
        producerArgsArray[i].numOfNews = stoi(conFileLines[j + 1]);
        producerArgsArray[i].queueSize = stoi(conFileLines[j + 2]);
        j += 3;
    }
    BoundedQueue* bq;
    pthread_t producerThreadsArray[numOfProducers];
    for (int i = 0; i < numOfProducers; i++) {
        bq = new BoundedQueue(producerArgsArray[i].queueSize);
        queueArray.push_back(bq);
        pthread_create(&producerThreadsArray[i], NULL, reinterpret_cast<void *(*)(void *)>(producer),
                       (void*)&producerArgsArray[i]);
    }
    pthread_t dispatcherThread;
    pthread_create(&dispatcherThread, NULL, reinterpret_cast<void *(*)(void *)>(dispatcher),
                   (void *)numOfProducers);
    mixedQueue = new BoundedQueue(editorQueueSize);
    pthread_t editorThreadsArray[3];
    for (int i = 0; i < 3; i++) {
        pthread_create(&editorThreadsArray[i], NULL, reinterpret_cast<void *(*)(void *)>(editor), (void *)i);
    }
    pthread_t screenManagerThread;
    pthread_create(&screenManagerThread, NULL, reinterpret_cast<void *(*)(void *)>(screenManager), nullptr);
    pthread_join(screenManagerThread, NULL);
    return 0;
}





