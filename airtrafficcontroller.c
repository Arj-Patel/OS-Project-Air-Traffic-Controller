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
#define DEPARTURE_MTYPE 174
#define ARRIVAL_MTYPE 175

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
    int status;  // 0 for takeoff complete, 1 for landing and deboarding/unloading complete
    Plane plane; // Plane details
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

    Plane planes[MAX_PLANES + 1];
    Airport airports[MAX_AIRPORTS + 1];

    file = fopen("AirTrafficController.txt", "a");
    if (file == NULL)
    {
        perror("fopen failed");
        exit(EXIT_FAILURE);
    }

    int isPresent[MAX_PLANES + 1] = {0};
    int hasDeparted[MAX_PLANES + 1] = {0};
    int hasArrived[MAX_PLANES + 1] = {0};
    int hasSentDepartedMsg[MAX_PLANES + 1] = {0};
    int hasSentArrivalMsg[MAX_PLANES + 1] = {0};

    while (1)
    {
        if (msgrcv(msgid, &message, sizeof(message), TERMINATION_MTYPE, IPC_NOWAIT) != -1)
        {
            if (message.terminate == 1)
            {
                break;
            }
        }

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
                // planes[i].mtype = i + 10;
                // if (msgsnd(msgid, &planes[i], sizeof(planes[i]), 0) == -1)
                // {
                //     perror("msgsnd failed");
                //     exit(EXIT_FAILURE);
                // }
                // isPresent[i] = 0;

                // Inform the departure airport
                if (!hasSentDepartedMsg[i])
                {
                    hasSentDepartedMsg[i] = 1;
                    airports[planes[i].departure_airport].mtype = i + 20;
                    airports[planes[i].departure_airport].airport_id = planes[i].departure_airport;
                    airports[planes[i].departure_airport].plane = planes[i]; // Add plane details
                    if (msgsnd(msgid, &airports[planes[i].departure_airport], sizeof(airports[planes[i].departure_airport]), 0) == -1)
                    {
                        perror("msgsnd failed");
                        exit(EXIT_FAILURE);
                    }
                }

                // Non-blocking receive for takeoff complete message from departure airport
                if (msgrcv(msgid, &airports[planes[i].departure_airport], sizeof(airports[planes[i].departure_airport]), i + 40, IPC_NOWAIT) != -1)
                {
                    if (airports[planes[i].departure_airport].status == 0)
                    {
                        hasDeparted[i] = 1;
                    }
                }

                // If the plane has departed, inform the arrival airport
                if (hasDeparted[i])
                {
                    if (!hasSentArrivalMsg[i])
                    {
                        hasSentArrivalMsg[i] = 1;
                        airports[planes[i].arrival_airport].mtype = i + 30;
                        airports[planes[i].arrival_airport].airport_id = planes[i].arrival_airport;
                        airports[planes[i].arrival_airport].plane = planes[i]; // Add plane details
                        if (msgsnd(msgid, &airports[planes[i].arrival_airport], sizeof(airports[planes[i].arrival_airport]), 0) == -1)
                        {
                            perror("msgsnd failed");
                            exit(EXIT_FAILURE);
                        }
                    }

                    // Non-blocking receive for landing and deboarding/unloading complete message from arrival airport
                    if (msgrcv(msgid, &airports[planes[i].arrival_airport], sizeof(airports[planes[i].arrival_airport]), i + 50, IPC_NOWAIT) != -1)
                    {
                        if (airports[planes[i].arrival_airport].status == 1)
                        {
                            hasArrived[i] = 1;
                        }
                    }
                }

                // If the plane has arrived, inform the plane process
                if (hasArrived[i])
                {
                    planes[i].mtype = i + 10;
                    if (msgsnd(msgid, &planes[i], sizeof(planes[i]), 0) == -1)
                    {
                        perror("msgsnd failed");
                        exit(EXIT_FAILURE);
                    }

                    // Reset the plane's status
                    hasDeparted[i] = 0;
                    hasArrived[i] = 0;
                    hasSentDepartedMsg[i] = 0;
                    hasSentArrivalMsg[i] = 0;
                }
            }
        }
    }

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