#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>

#define MAX_PLANES 10
#define MAX_AIRPORTS 10
#define TERMINATION_MTYPE 173

typedef struct
{
    long mtype;
    int plane_id;
    int plane_type;
    int total_weight;
    int num_passengers;
    int departure_airport;
    int arrival_airport;
} Plane;

typedef struct
{
    long mtype;
    int airport_id;
    int status; // 0 for takeoff complete, 1 for landing and deboarding/unloading complete
} Airport;

typedef struct
{
    long mtype;
    int terminate;
} Message;

int main()
{
    int num_airports;
    printf("Enter the number of airports to be handled/managed: ");
    scanf("%d", &num_airports);

    Plane plane;
    Airport airport;
    Message message;
    FILE *file;

    key_t key = ftok(".", 'a');
    int msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1)
    {
        perror("msgget failed");
        exit(EXIT_FAILURE);
    }

    Plane planes[MAX_PLANES+1];

    file = fopen("AirTrafficController.txt", "a");
    if (file == NULL)
    {
        perror("fopen failed");
        exit(EXIT_FAILURE);
    }

    while (1)
    {

        if (msgrcv(msgid, &message, sizeof(message), TERMINATION_MTYPE, IPC_NOWAIT) != -1)
        {
            if (message.terminate == 1)
            {
                break;
            }
        }

        int isPresent[MAX_PLANES] = {0};

        for (int i = 1; i <= MAX_PLANES; i++)
        {
            if (msgrcv(msgid, &planes[i], sizeof(planes[i]), i, IPC_NOWAIT) != -1)
            {
                isPresent[i] = 1;
            }
            else if (errno != ENOMSG)
            {
                perror("msgrcv failed");
                exit(EXIT_FAILURE);
            }
        }

        for (int i = 1; i <= MAX_PLANES; i++)
        {
            if (isPresent[i] == 0)
            {
                continue;
            }
            else
            {
                printf("here\n");
                planes[i].mtype = i+10;
                if(msgsnd(msgid, &planes[i], sizeof(planes[i]), 0) == -1)
                {
                    perror("msgsnd failed");
                    exit(EXIT_FAILURE);
                }
                isPresent[i] = 0;
            }
        }

        fprintf(file, "Plane %d has departed from Airport %d and will land at Airport %d.\n", plane.plane_id, plane.departure_airport, plane.arrival_airport);
    }

    // Send termination messages to all airports
    // Wait for confirmation messages about airport termination
    if (fclose(file) == EOF)
    {
        perror("fclose failed");
        exit(EXIT_FAILURE);
    }

    if (msgctl(msgid, IPC_RMID, NULL) == -1)
    {
        perror("msgctl failed");
        exit(EXIT_FAILURE);
    }

    return 0;
}