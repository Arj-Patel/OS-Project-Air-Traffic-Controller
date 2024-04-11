#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX_PASSENGERS 10
#define MAX_CARGO_ITEMS 100
#define MAX_WEIGHT 100
#define CREW_WEIGHT 75
#define MAX_AIRPORTS 10

typedef struct {
    long mtype;
    int plane_id;
    int plane_type;
    int total_weight;
    int num_passengers;
    int departure_airport;
    int arrival_airport;
} Plane;

int main() {
    int plane_id, plane_type, num_seats, num_cargo_items, avg_cargo_weight;
    int departure_airport, arrival_airport;
    int total_weight = 0;
    int i, luggage_weight, body_weight;
    int pipefd[2];
    pid_t pid;

    printf("Enter Plane ID: ");
    scanf("%d", &plane_id);

    printf("Enter Type of Plane: ");
    scanf("%d", &plane_type);

    if (plane_type == 1) {
        printf("Enter Number of Occupied Seats: ");
        scanf("%d", &num_seats);

        for (i = 0; i < num_seats; i++) {
            if (pipe(pipefd) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }

            pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            if (pid == 0) {
                close(pipefd[0]);

                printf("Enter Weight of Your Luggage: ");
                scanf("%d", &luggage_weight);

                printf("Enter Your Body Weight: ");
                scanf("%d", &body_weight);

                write(pipefd[1], &luggage_weight, sizeof(int));
                write(pipefd[1], &body_weight, sizeof(int));

                close(pipefd[1]);
                exit(EXIT_SUCCESS);
            } else {
                close(pipefd[1]);

                read(pipefd[0], &luggage_weight, sizeof(int));
                read(pipefd[0], &body_weight, sizeof(int));

                total_weight += luggage_weight + body_weight;

                close(pipefd[0]);
                wait(NULL);
            }
        }

        total_weight += num_seats * CREW_WEIGHT;
    } else {
        printf("Enter Number of Cargo Items: ");
        scanf("%d", &num_cargo_items);

        printf("Enter Average Weight of Cargo Items: ");
        scanf("%d", &avg_cargo_weight);

        total_weight = num_cargo_items * avg_cargo_weight + 2 * CREW_WEIGHT;
    }

    printf("Enter Airport Number for Departure: ");
    scanf("%d", &departure_airport);

    printf("Enter Airport Number for Arrival: ");
    scanf("%d", &arrival_airport);

    Plane plane = {1, plane_id, plane_type, total_weight, num_seats, departure_airport, arrival_airport};

    // Send a message to the air traffic controller with the plane details
    // Wait for a message from the air traffic controller indicating that the deboarding/unloading process is complete

    printf("Plane has successfully traveled from Airport %d to Airport %d!\n", departure_airport, arrival_airport);

    return 0;
}