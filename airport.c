#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <limits.h>

typedef struct {
    int loadCapacity;
    int isAvailable;
    pthread_mutex_t mutex;
} Runway;

typedef struct {
    int airportNumber;
    int numberOfRunways;
    Runway runways[11]; // 10 runways + 1 backup
} Airport;

typedef struct {
    int planeID;
    int totalWeight;
    int isArrival; // 1 for arrival, 0 for departure
} Plane;

Airport airport;

void* handlePlane(void* arg) {
    Plane* plane = (Plane*)arg;
    int bestFitIndex = -1;
    int minDiff = INT_MAX;

    // Find suitable runway and handle arrival or departure
    for (int i = 0; i < airport.numberOfRunways; i++) {
        if (pthread_mutex_trylock(&airport.runways[i].mutex) == 0) {
            int diff = airport.runways[i].loadCapacity - plane->totalWeight;
            if (diff >= 0 && diff < minDiff) {
                if (bestFitIndex != -1) {
                    pthread_mutex_unlock(&airport.runways[bestFitIndex].mutex);
                }
                bestFitIndex = i;
                minDiff = diff;
            } else {
                pthread_mutex_unlock(&airport.runways[i].mutex);
            }
        }
    }

    if (bestFitIndex == -1) {
        bestFitIndex = airport.numberOfRunways;
        pthread_mutex_lock(&airport.runways[bestFitIndex].mutex);
    }

    if (plane->isArrival) {
        sleep(2); // Simulate landing
        printf("Plane %d has landed on Runway No. %d of Airport No. %d\n", plane->planeID, bestFitIndex+1, airport.airportNumber);
        sleep(3); // Simulate deboarding/unloading
        printf("Plane %d has completed deboarding/unloading\n", plane->planeID);
    } else {
        sleep(3); // Simulate boarding/loading
        printf("Plane %d has completed boarding/loading on Runway No. %d of Airport No. %d\n", plane->planeID, bestFitIndex+1, airport.airportNumber);
        sleep(2); // Simulate takeoff
        printf("Plane %d has taken off\n", plane->planeID);
    }

    pthread_mutex_unlock(&airport.runways[bestFitIndex].mutex);
    pthread_exit(NULL);
}

int main() {
    // Initialize airport
    airport.airportNumber = 1; // Set airport number
    airport.numberOfRunways = 10; // Set number of runways

    // Initialize runways
    for (int i = 0; i < airport.numberOfRunways; i++) {
        airport.runways[i].loadCapacity = 1000; // Set load capacity
        pthread_mutex_init(&airport.runways[i].mutex, NULL); // Initialize mutex
    }

    // Initialize backup runway
    airport.runways[airport.numberOfRunways].loadCapacity = 2000; // Set load capacity
    pthread_mutex_init(&airport.runways[airport.numberOfRunways].mutex, NULL); // Initialize mutex

    // Set up message queue
    key_t key;
    int msgid;

    // ftok to generate unique key
    key_t key = ftok(".", 'a');
    int msgid = msgget(key, 0666 | IPC_CREAT);

    // Wait for messages from air traffic controller
    while (1) {
        Plane plane;
        msgrcv(msgid, &plane, sizeof(plane), 1, 0);

        // If termination message, break the loop
        if (plane.planeID == -1) break;

        // Create new thread to handle plane
        pthread_t thread;
        pthread_create(&thread, NULL, handlePlane, (void*)&plane);
    }

    
    return 0;
}