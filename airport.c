#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/msg.h>

typedef struct {
    int loadCapacity;
    int isAvailable;
} Runway;

typedef struct {
    int airportNumber;
    int numberOfRunways;
    Runway runways[11]; // 10 runways + 1 backup
} Airport;

typedef struct {
    int planeID;
    int totalWeight;
} Plane;

int main() {
    Airport airport;
    printf("Enter Airport Number: ");
    scanf("%d", &airport.airportNumber);
    printf("Enter number of Runways: ");
    scanf("%d", &airport.numberOfRunways);
    for (int i = 0; i < airport.numberOfRunways; i++) {
        printf("Enter loadCapacity of Runway %d: ", i+1);
        scanf("%d", &airport.runways[i].loadCapacity);
        airport.runways[i].isAvailable = 1;
    }
    // Initialize backup runway
    airport.runways[airport.numberOfRunways].loadCapacity = 15000;
    airport.runways[airport.numberOfRunways].isAvailable = 1;

    // Create message queue
    key_t key = ftok("queue", 65);
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

    // Cleanup and terminate process
    msgctl(msgid, IPC_RMID, NULL);
    return 0;
}

void* handlePlane(void* arg) {
    Plane* plane = (Plane*)arg;

    // Find suitable runway and handle arrival or departure
    // ...

    pthread_exit(NULL);
}