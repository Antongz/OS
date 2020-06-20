#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include <vector>
#include <bitset>
#include <algorithm>

#define DEBUG 0

using namespace std;

class Args
{
public:
    string _fileName;
    int _pageSize;
    int _frameNum;
    string _algo;

    int _regBitLen;
    int _interval;
    int _windowLen;

    // int _readTimes;
    int _writeTimes;// disk write time
    int _faultTimes;// disk fault time
    Args()
    {
        _regBitLen = 8;
        _interval = 1;
        _windowLen = 8;
    }
};

Args arg;

// a memory access command
class Cmd
{
public:
    char _RW;
    unsigned int _addr;
    unsigned int _pageNum;
};
vector<Cmd> cmdList;
int cmdIndex = 0;

// a frame in memroy
class Frame
{
public:
    int _index;
    unsigned char _shiftReg;
    unsigned int _pageNum;

    int _fifo;
    int _lru;

    char _rw;

    Frame(int i)
    {
        _index = i;
        _shiftReg = 0;
        _pageNum = -1;

        _fifo = -1;
        _lru = 0;
        _rw = 'R';
    }

    void hit(int a)
    {
        if (a != 0)
        {
            _shiftReg |= 1 << (a - 1);
        }
    }

    void old()
    {
        _shiftReg >>= 1;
    }

};

// compare by ARB
bool compARB(const Frame & a, const Frame & b)
{
    if (a._shiftReg < b._shiftReg)
    {
        return 1;
    }
    else if (a._shiftReg == b._shiftReg)
    {
        return a._fifo < b._fifo;
    }
    else
    {
        return 0;
    }
}

// compare by WSARB
bool compWSARB(const Frame & a, const Frame & b)
{
    int fa = 0, fb = 0;
    for (int i = 1 + cmdIndex - arg._windowLen; i <= cmdIndex; i++)
    {
        if (i < 0)
        {
            continue;
        }
        if (cmdList[i]._pageNum == a._pageNum)
        {
            fa++;
        }
        if (cmdList[i]._pageNum == b._pageNum)
        {
            fb++;
        }
    }


    if (fa < fb)
    {
        return 1;
    }
    else if (fa > fb)
    {
        return 0;
    }
    else
    {
        if (a._shiftReg < b._shiftReg)
        {
            return 1;
        }
        else if (a._shiftReg == b._shiftReg)
        {
            return a._fifo < b._fifo;
        }
        else
        {
            return 0;
        }
    }
}
// compare by FIFO
bool compFIFO(const Frame & a, const Frame & b)
{
    return a._fifo < b._fifo;
}
// compare by LRU
bool compLRU(const Frame & a, const Frame & b)
{
    return a._lru < b._lru;
}
// find a frame to replace
int FindVictim(Args &arg, vector<Frame> &frameList)
{
    // for (int i = frameList.size()-1; i >=0 ; i--)
    for (int i = 0; i < frameList.size(); i++)
    {
        if (frameList[i]._pageNum == -1)
        {
            return i;
        }
    }

    vector<Frame> flist = frameList;
    if (arg._algo == "FIFO")
    {
        sort(flist.begin(), flist.end(), compFIFO);
    }
    else if (arg._algo == "LRU")
    {
        sort(flist.begin(), flist.end(), compLRU);
    }
    else if (arg._algo == "ARB")
    {
        sort(flist.begin(), flist.end(), compARB);
    }
    else
    {
        sort(flist.begin(), flist.end(), compWSARB);
    }

    return flist[0]._index;

}
// simulate the access
void Run(Args &arg, vector<Frame> &frameList, vector<Cmd> &cmdList)
{
    int i;
    int victim;
    for (cmdIndex = 0; cmdIndex < cmdList.size(); cmdIndex++)
    {
        if (DEBUG)
        {
            cout << "Times:" << cmdIndex + 1 << "  ";
        }

        // old
        if (cmdIndex%arg._interval == 0)
        {
            for (int fdx = 0; fdx < frameList.size(); fdx++)
            {
                frameList[fdx].old();
            }
        }

        Cmd cmd = cmdList[cmdIndex];
        for (i = 0; i < frameList.size(); i++)
        {
            if (frameList[i]._pageNum == cmd._pageNum)
            {
                break;
            }
        }
        if (i == frameList.size())
        {
            arg._faultTimes++;
            if (DEBUG)
            {
                cout << "Miss Page " << cmd._pageNum << "  ";
            }
            // find Victim
            victim = FindVictim(arg, frameList);

            if (DEBUG)
            {
                cout << "Replace page " << frameList[victim]._pageNum << " ";
            }
            if (frameList[victim]._rw == 'W')
            {
                arg._writeTimes++;
            }

            frameList[victim]._pageNum = cmd._pageNum;
            frameList[victim]._rw = cmd._RW;
            frameList[victim]._fifo = cmdIndex;
            frameList[victim]._lru = cmdIndex;

            frameList[victim]._shiftReg = 0;// clear the shift bit
            frameList[victim].hit(arg._regBitLen);

        }
        else
        {
            // Hit
            frameList[i].hit(arg._regBitLen);
            frameList[i]._lru = cmdIndex;
            if (cmd._RW == 'W')
            {
                frameList[i]._rw = cmd._RW;
            }
            if (DEBUG)
            {
                cout << "Hit Page " << frameList[i]._pageNum << "  ";
            }
        }
        if (DEBUG)
        {
            cout << " frames ";
            for (int fdx = 0; fdx < frameList.size(); fdx++)
            {
                cout << frameList[fdx]._pageNum << " " << frameList[fdx]._rw << " " << bitset<8 >(frameList[fdx]._shiftReg) << "   ";
            }
            cout << endl;
        }
    }
}
// read the file
void ReadFile(Args &arg, vector<Cmd> &cmdList)
{
    Cmd cmd;
    char line[100];
    char * p;
    FILE * file = fopen(arg._fileName.c_str(), "r");
    while (1)
    {
        p = fgets(line, 100, file);
        if (p == NULL)
        {
            break;
        }
        if (line[0] == 'R' || line[0] == 'W')
        {
            // cout << line;
            sscanf(line, "%c%x", &cmd._RW, &cmd._addr);
            cmd._pageNum = cmd._addr / arg._pageSize;
            cmdList.push_back(cmd);
        }
    }
    fclose(file);
}
int main(int argc, char *argv[])
{

    if (argc < 5)
    {
        printf("Wrong\n");
        return -1;
    }

    //arg._fileName = "input.txt";
    arg._fileName = argv[1];

    //arg._pageSize = 4096;
    arg._pageSize = atoi(argv[2]);
    //arg._frameNum = 3;
    arg._frameNum = atoi(argv[3]);
    //arg._algo = "WSARB";// FIFO / LRU / ARB / WSARB
    arg._algo = argv[4];

    if (arg._algo == "ARB" || arg._algo == "WSARB")
    {
        //arg._regBitLen = 4;
        arg._regBitLen = atoi(argv[5]);
        //arg._interval = 3;
        arg._interval = atoi(argv[6]);
    }
    if (arg._algo == "WSARB")
    {
        arg._windowLen = atoi(argv[7]);
    }

    // arg._readTimes = 0;
    arg._writeTimes = 0;
    arg._faultTimes = 0;


    ReadFile(arg, cmdList);

    vector<Frame> frameList;
    for (int i = 0; i < arg._frameNum; i++)
    {
        frameList.push_back(Frame(i));
    }

    Run(arg, frameList, cmdList);

    printf("events in trace:    %d\n", cmdList.size());
    printf("total disk reads:   %d\n", arg._faultTimes);
    printf("total disk writes:  %d\n", arg._writeTimes);
    printf("page faults:        %d\n", arg._faultTimes);


    return 0;
}


