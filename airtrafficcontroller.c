#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX_PLANES 10
#define MAX_AIRPORTS 100

typedef struct {
    long mtype;
    int plane_id;
    int plane_type;
    int total_weight;
    int num_passengers;
    int departure_airport;
    int arrival_airport;
} Plane;

typedef struct {
    long mtype;
    int airport_id;
    int status; // 0 for takeoff complete, 1 for landing and deboarding/unloading complete
} Airport;

int main() {
    int num_airports;
    printf("Enter the number of airports to be handled/managed: ");
    scanf("%d", &num_airports);

    Plane plane;
    Airport airport;
    FILE *file;

    key_t key = ftok(".", 'a');
    int msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        if (msgrcv(msgid, &plane, sizeof(plane), 1, 0) == -1) {
            perror("msgrcv failed");
            exit(EXIT_FAILURE);
        }

        if (plane.plane_id == -1) {
            break;
        }

        // Inform the departure airport to begin the boarding/loading and the takeoff process
        airport.mtype = plane.departure_airport;
        airport.airport_id = plane.departure_airport;
        airport.status = 0;
        if (msgsnd(msgid, &airport, sizeof(airport), 0) == -1) {
            perror("msgsnd failed");
            exit(EXIT_FAILURE);
        }

        // Wait for a message from the departure airport indicating that the takeoff process is complete
        if (msgrcv(msgid, &airport, sizeof(airport), plane.departure_airport, 0) == -1) {
            perror("msgrcv failed");
            exit(EXIT_FAILURE);
        }

        // Inform the arrival airport regarding the arrival of the plane
        airport.mtype = plane.arrival_airport;
        airport.airport_id = plane.arrival_airport;
        airport.status = 1;
        if (msgsnd(msgid, &airport, sizeof(airport), 0) == -1) {
            perror("msgsnd failed");
            exit(EXIT_FAILURE);
        }

        // Wait for a message from the arrival airport indicating that the landing and deboarding/unloading process is complete
        if (msgrcv(msgid, &airport, sizeof(airport), plane.arrival_airport, 0) == -1) {
            perror("msgrcv failed");
            exit(EXIT_FAILURE);
        }

        // Inform the plane process
        if (msgsnd(msgid, &plane, sizeof(plane), 0) == -1) {
            perror("msgsnd failed");
            exit(EXIT_FAILURE);
        }

        file = fopen("AirTrafficController.txt", "a");
        if (file == NULL) {
            perror("fopen failed");
            exit(EXIT_FAILURE);
        }
        fprintf(file, "Plane %d has departed from Airport %d and will land at Airport %d.\n", plane.plane_id, plane.departure_airport, plane.arrival_airport);
        if (fclose(file) == EOF) {
            perror("fclose failed");
            exit(EXIT_FAILURE);
        }
    }

    // Send termination messages to all airports
    // Wait for confirmation messages about airport termination

    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("msgctl failed");
        exit(EXIT_FAILURE);
    }

    return 0;
}