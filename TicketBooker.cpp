#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <list>
#include <string>
#include <fstream>
#include <queue>

using namespace std;

// #define DEBUG 

class Customer
{
public:
    char id[20];             // id
    int arrivalTime;
    int priority;
    int age;                // age/run
    int ticketRequired;     // how many ticket that required
    int readyTime;
    int endTime;
    int remainTickets;      // remain time quantitum in current run
    int running;            // runnint time
    int gapStartTime;       // when does current run start
};

// show customer information
void printCustomer(Customer & cus)
{
    printf("%s %10d %12d %12d %12d %12d\n",
        cus.id, cus.arrivalTime, cus.ticketRequired,
        cus.running, cus.priority, cus.age);
}

// display 4 queue, debug only
void showQueue(int ticket, list<Customer> &queue1, list<Customer>  &queue2, list<Customer>  &queue3, list<Customer>  &queueLow)
{
#ifdef DEBUG
    printf("Time: %-3d\n", ticket * 5);
    printf("name arrival_time tickets_required running priority_number age/runs\n");
    printf("queue1\n");
    for_each(queue1.begin(), queue1.end(), printCustomer);
    for_each(queue2.begin(), queue2.end(), printCustomer);
    for_each(queue3.begin(), queue3.end(), printCustomer);
    printf("queue2\n");
    for_each(queueLow.begin(), queueLow.end(), printCustomer);
    printf("\n");
#endif // !DEBGB
}

// display the running result
void showResult(vector<Customer> & finishVec)
{
    printf("name   arrival   end   ready   running   waiting\n");
    for (int i = 0; i < finishVec.size(); i++)
    {
        Customer cus = finishVec[i];
        printf("%s%10d%10d%10d%10d%10d\n",
            cus.id, cus.arrivalTime, cus.endTime, cus.readyTime,
            cus.running, cus.endTime - cus.readyTime - cus.running);
    }
}

// starve. when one process run, all the customer in low queue decrease by 1
void updateAge(Customer & cus, list<Customer>  &queueLow, vector<Customer> & upGradeVec)
{
    list<Customer> tempList;
    for (list<Customer>::iterator iter = queueLow.begin(); iter != queueLow.end(); iter++)
    {
        if (iter->arrivalTime <= cus.gapStartTime && strcmp(iter->id, cus.id) != 0)
        {
            iter->age++;
            if (iter->age >= 8)
            {
                iter->age = 0;
                iter->priority--;
                if (iter->priority <= 3)
                {
                    upGradeVec.push_back(*iter);
                    continue;
                }
            }
        }
        tempList.push_back(*iter);
    }
    queueLow = tempList;
}

// read the data in file
void readData(vector<Customer> & customerVec, char * path)
{
    ifstream fin(path);
    Customer cus;
    while (fin >> cus.id >> cus.arrivalTime >> cus.priority >> cus.age >> cus.ticketRequired)
    {
        cus.remainTickets = 0;
        cus.running = 0;
        cus.readyTime = -1;
        cus.endTime = 0;
        cus.gapStartTime = 0;
        customerVec.push_back(cus);
    }
    fin.close();

    // sort the customer by id
    for (int i = 0; i < customerVec.size(); i++)
    {
        for (int j = 0; j < customerVec.size() - i - 1; j++)
        {
            if (customerVec[j].id > customerVec[j + 1].id)
            {
                Customer c = customerVec[j];
                customerVec[j] = customerVec[j + 1];
                customerVec[j + 1] = c;
            }
        }
    }
}

// main loop
void run(char * path)
{
    // global queue
    vector<Customer> finishVec;
    list<Customer> queue1;
    list<Customer> queue2;
    list<Customer> queue3;
    list<Customer> queueLow;

    vector<Customer> allCustomerVec;
    readData(allCustomerVec, path);

    // temporary queue
    vector<Customer> upGradeVec;
    vector<Customer> newCommingVec;
    vector<Customer> timeCompleteHighVec;
    vector<Customer> timeCompleteLowVec;

    list<Customer> *pQueue;
    int bSelectNewCus = 1;

    // temporary variable
    vector<Customer> tempVec;
    Customer cus;

    int ticket = 0;
    for (ticket = 0; ticket < 100000; ticket++)
    {
        int time = ticket * 5;
#pragma region START
        // newly arrived customer
        tempVec.clear();
        newCommingVec.clear();
        for (int i = 0; i < allCustomerVec.size(); i++)
        {
            cus = allCustomerVec[i];
            if (cus.arrivalTime == time)
            {
                newCommingVec.push_back(cus);
            }
            else
            {
                tempVec.push_back(cus);
            }
        }
        allCustomerVec = tempVec;

        int highEmpty1 = queue1.empty() && queue2.empty() && queue3.empty();

        // 1. high priority queue，new costumer incoming
        for (int i = 0; i < newCommingVec.size(); i++)
        {
            cus = newCommingVec[i];
            switch (cus.priority)
            {
            case 1:
                queue1.push_back(cus);
                break;
            case 2:
                queue2.push_back(cus);
                break;
            case 3:
                queue3.push_back(cus);
                break;
            default:
                break;
            }
        }
        // newCommingVec.clear();

        // 2. high priority queue，time quantitum run out
        if (!timeCompleteHighVec.empty())
        {
            cus = timeCompleteHighVec[0];
            switch (cus.priority)
            {
            case 1:
                queue1.push_back(cus);
                break;
            case 2:
                queue2.push_back(cus);
                break;
            case 3:
                queue3.push_back(cus);
                break;
            default:
                break;
            }
        }
        timeCompleteHighVec.clear();

        // 3. high priority queue，upgrade customer
        if (!upGradeVec.empty())
        {
            for (int i = 0; i < upGradeVec.size(); i++)
            {
                cus = upGradeVec[i];
                queue3.push_back(cus);
            }
        }
        upGradeVec.clear();

        int highEmpty2 = queue1.empty() && queue2.empty() && queue3.empty();
        if (highEmpty1 && !highEmpty2)
        {
            // high priority queue, interrupt the low priority queue
            if (!queueLow.empty())
            {
                cus = queueLow.front();
                if (cus.remainTickets > 0)
                {
                    cus.remainTickets = 0;
                    queueLow.pop_front();
                    // queueLow.push_front(cus);
                    timeCompleteLowVec.push_back(cus);
                    updateAge(cus, queueLow, upGradeVec);
#ifdef DEBUG
                    printf("\n\n--------- Interrupt  -------------\n\n");
#endif
                }
            }
        }

        // 1. low priority queue，add the customer time quantitum run out
        if (!timeCompleteLowVec.empty())
        {
            cus = timeCompleteLowVec[0];
            queueLow.push_back(cus);
        }
        timeCompleteLowVec.clear();

        // 2. low priority queue, newly arrived customer
        for (int i = 0; i < newCommingVec.size(); i++)
        {
            cus = newCommingVec[i];
            if (cus.priority > 3)
            {
                queueLow.push_back(cus);
            }
        }
        newCommingVec.clear();
#pragma endregion
        // display the queue
        showQueue(ticket, queue1, queue2, queue3, queueLow);

        // select a customer from the queue
        if (bSelectNewCus)
        {
            if (!queue1.empty())
            {
                pQueue = &queue1;
            }
            else if (!queue2.empty())
            {
                pQueue = &queue2;
            }
            else if (!queue3.empty())
            {
                pQueue = &queue3;
            }
            else
            {
                pQueue = &queueLow;
            }
        }

        if (pQueue != &queueLow)
        {

#pragma region high priority queue

            // high priority queue execute
            cus = pQueue->front();
            pQueue->pop_front();

            if (cus.remainTickets == 0)
            {
                cus.remainTickets = (8 - cus.priority) * 10 / 5;
                cus.gapStartTime = time;// record the start time
            }
            bSelectNewCus = 0;

            cus.readyTime = cus.readyTime == -1 ? time : cus.readyTime;
            cus.running += 5;
            cus.ticketRequired--;
            if (cus.ticketRequired == 0)
            {
                // task complete
                cus.endTime = time + 5;
                finishVec.push_back(cus);
                bSelectNewCus = 1;
                updateAge(cus, queueLow, upGradeVec); // task complete, increase the age
            }
            else
            {
                cus.remainTickets--;
                // 
                if (cus.remainTickets == 0)
                {
#pragma region time quantitum run out
                    bSelectNewCus = 1;
                    updateAge(cus, queueLow, upGradeVec); //  increase the age      
                    cus.age++;
                    if (cus.age == 3)
                    {
                        cus.age = 0;
                        cus.priority++;
                        switch (cus.priority)
                        {
                        case 2:
                        case 3:
                            timeCompleteHighVec.push_back(cus);
                            break;
                        case 4:
                            // cus.age = -1;
                            timeCompleteLowVec.push_back(cus);
                            break;
                        default:
                            printf("Error\n");
                            break;
                        }
                    }
                    else
                    {
                        timeCompleteHighVec.push_back(cus);
                    }
#pragma endregion
                }
                else
                {
#pragma region 时间片没有用完
                    bSelectNewCus = 0;
                    // not finish, put to the front
                    pQueue->push_front(cus);
#pragma endregion
                }
            }
#pragma endregion
        }
        else if (pQueue->empty() && allCustomerVec.empty())
        {
            // all the task finished
            break;
        }
        else if (pQueue->empty())
        {
            continue;
        }
        else
        {
#pragma region low priority queue

            cus = pQueue->front();
            pQueue->pop_front();
            cus.age = 0;

            // 
            if (cus.remainTickets == 0)
            {
                cus.remainTickets = 20;
                cus.gapStartTime = time;
            }

            cus.readyTime = cus.readyTime == -1 ? time : cus.readyTime;
            cus.running += 5;
            cus.ticketRequired--;

            if (cus.ticketRequired == 0)
            {
                // task finish
                cus.endTime = time + 5;
                finishVec.push_back(cus);
                updateAge(cus, queueLow, upGradeVec); // increase the age    
            }
            else
            {
                cus.remainTickets--;

                // 是否运行一次20周期
                if (cus.remainTickets == 0)
                {
                    // time quantitum run out
                    // cus.age = -1;
                    // move the the tail of the queue
                    timeCompleteLowVec.push_back(cus);
                    updateAge(cus, queueLow, upGradeVec);  // task finish   
                }
                else
                {
                    // move to the head
                    pQueue->push_front(cus);
                }
            }
#pragma endregion
        }


    }// !while (1)

    showResult(finishVec);
}

int main(int argc, char *argv[])
{
    run(argv[1]);
    return 0;
}


