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
#define TERMINATION_MTYPE 5270

typedef struct
{
    long mtype;
    int takeOff;
} TakeOffMessage;

typedef struct
{
    long mtype;
    int deboardingComplete;
} DeboardingMessage;

typedef struct
{
    long mtype;
    int plane_id;
    int plane_type;
    int total_weight;
    int num_passengers;
    int departure_airport;
    int arrival_airport;
    int terminate;
} Plane;

// typedef struct
// {
//     long mtype;
//     int airport_id;
//     int status;  // 0 for takeoff complete, 1 for landing and deboarding/unloading complete
//     Plane plane; // Plane details
// } Airport;

typedef struct
{
    long mtype;
    int terminate;
} TerminateMessage;

int main()
{
    int num_airports;
    printf("Enter the number of airports to be handled/managed: ");
    scanf("%d", &num_airports);

    Plane plane;
    // Airport airport;
    TerminateMessage message;
    int startTermination = 0;
    int toBreak = 0;
    FILE *file;

    key_t key = ftok(".", 'a');
    int msgid = msgget(key, 0666 | IPC_CREAT);
    if (msgid == -1)
    {
        perror("msgget failed");
        exit(EXIT_FAILURE);
    }

    Plane planes[MAX_PLANES + 1];
    // Airport airports[MAX_AIRPORTS + 1];

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

    DeboardingMessage dbmsg;
    TakeOffMessage tkoffmsg;

    while (1)
    {
        if (msgrcv(msgid, &message, sizeof(message), TERMINATION_MTYPE, IPC_NOWAIT) != -1)
        {
            startTermination = 1;
        }

        for (int i = 1; i <= MAX_PLANES; i++)
        {
            // if (startTermination != 1)
            // {
            if (msgrcv(msgid, &planes[i], sizeof(planes[i]), i, IPC_NOWAIT) != -1)
            {
                printf("plane %d asked to depart\n", i);
                if (!startTermination)
                {
                    isPresent[i] = 1;
                }
                else
                {
                    printf("plane %d asked to depart but ATC has started termination process\n", i);
                    planes[i].terminate = 1;
                    planes[i].mtype = i + 10;
                    if (msgsnd(msgid, &planes[i], sizeof(planes[i]), 0) == -1)
                    {
                        perror("msgsnd failed");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            else if (errno != ENOMSG)
            {
                perror("msgrcv failed");
                exit(EXIT_FAILURE);
            }
            // }
        }

        for (int i = 1; i <= MAX_PLANES; i++)
        {
            if (isPresent[i] == 0)
            {
                continue;
            }
            else
            {

                // Inform the departure airport to depart the plane
                if (!hasSentDepartedMsg[i])
                {
                    printf("ATC sent departure message %d\n", i);
                    hasSentDepartedMsg[i] = 1;
                    // airports[planes[i].departure_airport].mtype = i + 40 + (planes[i].departure_airport - 1) * 10;
                    // airports[planes[i].departure_airport].airport_id = planes[i].departure_airport;
                    // airports[planes[i].departure_airport].plane = planes[i]; // Add plane details
                    planes[i].mtype = i + 40 + (planes[i].departure_airport - 1) * 10;
                    if (msgsnd(msgid, &planes[i], sizeof(planes[i]), 0) == -1)
                    {
                        perror("msgsnd failed");
                        exit(EXIT_FAILURE);
                    }
                }
                // TODO: check if we recieve the message from the airport. DONE
                //  Non-blocking receive for takeoff complete message from departure airport i.e. the plane has departed
                //  printf("checking\n");
                //  if (msgrcv(msgid, &airports[planes[i].departure_airport], sizeof(airports[planes[i].departure_airport]), i + 20, IPC_NOWAIT) != -1)
                //  {
                //      printf("plane departed message recieved\n");
                //      if (airports[planes[i].departure_airport].status == 0)
                //      {
                //          fprintf(file, "Plane %d has departed from Airport %d and will land at Airport %d.\n",
                //                  planes[i].plane_id,
                //                  planes[i].departure_airport,
                //                  planes[i].arrival_airport);
                //          hasDeparted[i] = 1;
                //      }
                //  }

                if (msgrcv(msgid, &tkoffmsg, sizeof(tkoffmsg), i + 20, IPC_NOWAIT) != -1)
                {
                    printf("plane departed message received %d\n", i);
                    if (tkoffmsg.takeOff == 1)
                    {
                        fprintf(file, "Plane %d has departed from Airport %d and will land at Airport %d.\n",
                                planes[i].plane_id,
                                planes[i].departure_airport,
                                planes[i].arrival_airport);
                        hasDeparted[i] = 1;
                    }
                }

                // If the plane has departed, inform the arrival airport
                if (hasDeparted[i])
                {
                    if (!hasSentArrivalMsg[i])
                    {
                        printf("ATC sent arrival message %d\n", i);
                        hasSentArrivalMsg[i] = 1;
                        // airports[planes[i].arrival_airport].mtype = i + 140 + (planes[i].arrival_airport - 1) * 10;
                        // airports[planes[i].arrival_airport].airport_id = planes[i].arrival_airport;
                        // airports[planes[i].arrival_airport].plane = planes[i]; // Add plane details
                        planes[i].mtype = i + 140 + (planes[i].arrival_airport - 1) * 10;
                        if (msgsnd(msgid, &planes[i], sizeof(planes[i]), 0) == -1)
                        {
                            perror("msgsnd failed");
                            exit(EXIT_FAILURE);
                        }
                    }

                    // Non-blocking receive for landing and deboarding/unloading complete message from arrival airport
                    if (msgrcv(msgid, &dbmsg, sizeof(dbmsg), i + 30, IPC_NOWAIT) != -1)
                    {
                        printf("plane arrived message %d\n", i);
                        // if (airports[planes[i].arrival_airport].status == 1)
                        // {
                        hasArrived[i] = 1;
                        // }
                    }
                }

                // If the plane has arrived, inform the plane process
                // TODO: check whether ATC is sending the below message or not. i.e. check if hasArrived[i] is 1 or not in line 181. DONE
                if (hasArrived[i])
                {
                    planes[i].mtype = i + 10;
                    if (msgsnd(msgid, &planes[i], sizeof(planes[i]), 0) == -1)
                    {
                        printf("ATC sent plane arrived message to plane %d\n", i);
                        perror("msgsnd failed");
                        exit(EXIT_FAILURE);
                    }

                    // Reset the plane's status
                    hasDeparted[i] = 0;
                    hasArrived[i] = 0;
                    hasSentDepartedMsg[i] = 0;
                    hasSentArrivalMsg[i] = 0;
                    isPresent[i] = 0;
                }
            }
        }
        if (startTermination)
        {
            toBreak = 1;
            for (int i = 0; i < MAX_PLANES; i++)
            {
                if (isPresent[i])
                {
                    toBreak = 0;
                    break;
                }
            }
        }
        if (toBreak)
        {
            break;
        }
    }

    for (int i = 1; i <= MAX_AIRPORTS; i++)
    {
        // send terminate message to all airports
        message.mtype = i + 250;
        if (msgsnd(msgid, &message, sizeof(message), 0) == -1)
        {
            perror("msgsnd failed");
            exit(EXIT_FAILURE);
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