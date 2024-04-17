#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <stdbool.h>

#define MAX_THREADS 1000
#define MAX_PLANES 100

pthread_t threads[MAX_THREADS];
int thread_count = 0;
bool hasFinished[MAX_THREADS];
pthread_mutex_t hasFinishedMutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct
{
    int loadCapacity;
    int isAvailable;
    pthread_mutex_t mutex;
} Runway;

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
    int airportNumber;
    int numberOfRunways;
    Runway runways[11];
} AirportDetails;

AirportDetails airportDetails;

void *handlePlane(void *arg)
{
    Plane *plane = (Plane *)arg;
    int bestFitIndex = -1;
    int overWeight = 1;
    int minDiff = INT_MAX;

    // Find suitable runway and handle arrival or departure
    // for (int i = 0; i < airportDetails.numberOfRunways; i++)
    // {
    //     if (pthread_mutex_trylock(&airportDetails.runways[i].mutex) == 0)
    //     {
    //         int diff = airportDetails.runways[i].loadCapacity - plane->total_weight;
    //         if (diff >= 0 && diff < minDiff)
    //         {
    //             if (bestFitIndex != -1)
    //             {
    //                 pthread_mutex_unlock(&airportDetails.runways[bestFitIndex].mutex);
    //             }
    //             bestFitIndex = i;
    //             minDiff = diff;
    //         }
    //         else
    //         {
    //             pthread_mutex_unlock(&airportDetails.runways[i].mutex);
    //         }
    //     }
    // }

    for (int i = 0; i < airportDetails.numberOfRunways; i++)
    {
        if (plane->total_weight < airportDetails.runways[i].loadCapacity)
        {
            int diff = airportDetails.runways[i].loadCapacity - plane->total_weight;
            if(diff < minDiff){
                bestFitIndex = i;
                minDiff = diff;
            }
            overWeight = 0;
        }
    }

    if (overWeight)
    {
        bestFitIndex = airportDetails.numberOfRunways;
        pthread_mutex_lock(&airportDetails.runways[bestFitIndex].mutex);
    }
    else
    {
        // while (bestFitIndex == -1)
        // {
            pthread_mutex_lock(&airportDetails.runways[bestFitIndex].mutex);
            // for (int i = 0; i < airportDetails.numberOfRunways; i++)
            // {
            //     // printf("plane %d waiting", plane->plane_id);
            //     if (pthread_mutex_trylock(&airportDetails.runways[i].mutex) == 0)
            //     {
            //         int diff = airportDetails.runways[i].loadCapacity - plane->total_weight;
            //         if (diff >= 0 && diff < minDiff)
            //         {
            //             if (bestFitIndex != -1)
            //             {
            //                 pthread_mutex_unlock(&airportDetails.runways[bestFitIndex].mutex);
            //             }
            //             bestFitIndex = i;
            //             minDiff = diff;
            //         }
            //         else
            //         {
            //             pthread_mutex_unlock(&airportDetails.runways[i].mutex);
            //         }
            //     }
            // }

            // If no suitable runway was found, sleep for a short period before the next iteration
        // }
    }

    // if (bestFitIndex == -1)
    // {
    //     bestFitIndex = airportDetails.numberOfRunways;
    //     pthread_mutex_lock(&airportDetails.runways[bestFitIndex].mutex);
    // }

    if (plane->arrival_airport == airportDetails.airportNumber)
    {
        sleep(20); // Simulate landing
        printf("Plane %d has landed on Runway No. %d of Airport No. %d\n", plane->plane_id, bestFitIndex + 1, airportDetails.airportNumber);
        sleep(3); // Simulate deboarding/unloading
        printf("Plane %d has completed deboarding/unloading\n", plane->plane_id);
    }
    else
    {
        sleep(30); // Simulate boarding/loading
        printf("Plane %d has completed boarding/loading on Runway No. %d of Airport No. %d\n", plane->plane_id, bestFitIndex + 1, airportDetails.airportNumber);
        sleep(2); // Simulate takeoff
        printf("Plane %d has taken off\n", plane->plane_id);
    }

    pthread_mutex_unlock(&airportDetails.runways[bestFitIndex].mutex);
    // printf("here1\n");
    // pthread_exit(NULL);
}

void *handleArrival(void *arg)
{
    Plane *plane = (Plane *)arg;
    // Plane *plane = &airport->plane;
    key_t key = ftok(".", 'a');
    int msgid = msgget(key, 0666);

    // Handle landing and deboarding/unloading
    sleep(60);
    handlePlane(plane);
    // TODO: make this a simple hasArrived message DONE
    //  airport->mtype = plane->plane_id + 30; // Send landing and deboarding/unloading complete message
    //  if (msgsnd(msgid, airport, sizeof(*airport), 0) == -1)
    //  {
    //      perror("msgsnd failed");
    //      exit(1);
    //  }

    DeboardingMessage msg;
    msg.mtype = plane->plane_id + 30;
    msg.deboardingComplete = 1;

    if (msgsnd(msgid, &msg, sizeof(msg), 0) == -1)
    {
        perror("msgsnd failed");
        exit(1);
    }

    pthread_mutex_lock(&hasFinishedMutex);
    hasFinished[plane->plane_id] = true;
    pthread_mutex_unlock(&hasFinishedMutex);
    pthread_exit(NULL);
}

void *handleDeparture(void *arg)
{
    printf("in handleDeparture\n");
    Plane *plane = (Plane *)arg;
    // Plane *plane = &airport->plane;
    key_t key = ftok(".", 'a');
    int msgid = msgget(key, 0666);

    // Handle takeoff
    handlePlane(plane);
    // printf("here\n");
    // TODO: check whether the ATC recieves this message and continue from there DONE
    // airport->mtype = plane->plane_id + 20; // Send takeoff complete message
    // if (msgsnd(msgid, airport, sizeof(*airport), 0) != -1)
    // {
    //     printf("Sent takeoff complete message to ATC\n");
    // }
    // else
    // {
    //     perror("msgsnd failed");
    //     exit(1);
    // }
    TakeOffMessage msg;
    msg.mtype = plane->plane_id + 20; // or any other type you want
    msg.takeOff = 1;

    if (msgsnd(msgid, &msg, sizeof(msg), 0) != -1)
    {
        printf("Sent takeoff complete message to ATC\n");
    }
    else
    {
        perror("msgsnd failed");
        exit(1);
    }

    pthread_mutex_lock(&hasFinishedMutex);
    hasFinished[plane->plane_id] = true;
    pthread_mutex_unlock(&hasFinishedMutex);

    pthread_exit(NULL);
}

int main()
{
    // Initialize airport
    printf("Enter Airport Number: ");
    scanf("%d", &airportDetails.airportNumber);

    printf("Enter number of Runways: ");
    scanf("%d", &airportDetails.numberOfRunways);

    // Initialize runways
    for (int i = 0; i < airportDetails.numberOfRunways; i++)
    {
        printf("Enter loadCapacity of Runway %d: ", i + 1);
        scanf("%d", &airportDetails.runways[i].loadCapacity);
        pthread_mutex_init(&airportDetails.runways[i].mutex, NULL); // Initialize mutex
    }

    // Initialize backup runway
    airportDetails.runways[airportDetails.numberOfRunways].loadCapacity = 15000;             // Set load capacity
    pthread_mutex_init(&airportDetails.runways[airportDetails.numberOfRunways].mutex, NULL); // Initialize mutex

    // Set up message queue
    // key_t key;
    // int msgid;

    // ftok to generate unique key
    key_t key = ftok(".", 'a');
    int msgid = msgget(key, 0666 | IPC_CREAT);
    int temp_plane_id;
    Plane planes[MAX_PLANES + 1];

    int messageReceived[MAX_PLANES + 1] = {0};
    // Wait for messages from air traffic controller
    while (1)
    {
        // printf("heaer\n");
        // Airport airport;
        int mtype;
        for (mtype = 41 + 10 * (airportDetails.airportNumber - 1); mtype <= 40 + 10 * airportDetails.airportNumber; mtype++)
        {
            // printf("here\n");
            temp_plane_id = mtype % 10 == 0 ? 10 : mtype % 10;
            if (msgrcv(msgid, &planes[temp_plane_id], sizeof(planes[temp_plane_id]), mtype, IPC_NOWAIT) != -1)
            {
                // Handle message
                printf("Received message from air traffic controller%d\n", mtype);
                messageReceived[temp_plane_id] = 1;
                break;
            }
        }
        for (mtype = 141 + 10 * (airportDetails.airportNumber - 1); mtype <= 140 + 10 * airportDetails.airportNumber; mtype++)
        {
            // printf("here\n");
            temp_plane_id = mtype % 10 == 0 ? 10 : mtype % 10;
            if (msgrcv(msgid, &planes[temp_plane_id], sizeof(planes[temp_plane_id]), mtype, IPC_NOWAIT) != -1)
            {
                // Handle message
                printf("Received message from air traffic controller %d\n", temp_plane_id);
                messageReceived[temp_plane_id] = 1;
                break;
            }
        }

        for (int i = 1; i <= MAX_PLANES; i++)
        {
            if (messageReceived[i])
            {

                if (thread_count >= MAX_THREADS)
                {
                    fprintf(stderr, "Error: too many threads\n");
                    exit(1);
                }

                // If termination message, break the loop
                // if (airport.plane.plane_id == -1)
                //     break;

                // Create new thread to handle airport
                pthread_t thread;
                if (planes[i].mtype > 40 + 10 * (airportDetails.airportNumber - 1) && planes[i].mtype <= 40 + 10 * airportDetails.airportNumber)
                {
                    printf("departure\n");
                    pthread_create(&threads[thread_count++], NULL, handleDeparture, (void *)&planes[i]);
                }
                else if (planes[i].mtype > 140 + 10 * (airportDetails.airportNumber - 1) && planes[i].mtype <= 140 + 10 * airportDetails.airportNumber)
                {

                    pthread_create(&threads[thread_count++], NULL, handleArrival, (void *)&planes[i]);
                }
                messageReceived[i] = 0;
            }
        }
        // pthread_mutex_lock(&hasFinishedMutex);
        // for (int i = 0; i < thread_count; i++)
        // {
        //     if (hasFinished[i])
        //     {
        //         pthread_join(threads[i], NULL);
        //         // Shift remaining threads down
        //         for (int j = i; j < thread_count - 1; j++)
        //         {
        //             threads[j] = threads[j + 1];
        //             hasFinished[j] = hasFinished[j + 1];
        //         }
        //         thread_count--;
        //         i--; // Decrement i to account for the removed thread
        //     }
        // }
        // pthread_mutex_unlock(&hasFinishedMutex);
    }

    for (int i = 0; i < thread_count; i++)
    {
        pthread_join(threads[i], NULL);
    }

    return 0;
}