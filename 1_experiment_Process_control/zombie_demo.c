#include<stdio.h>
#include<unistd.h>
#include<iostream>
using namespace std;

int main()
{
    int pid = fork();
    if(pid<0)
    {
        cout << "fork error"<< endl;
        return 1;
    }
    else if(pid == 0)
    {
        sleep(2);
        cout << "child done"<<endl;
        return 0;
    }
    else
    {
        sleep(10);
        cout << "done"<<endl;
    }
    return 0;
}
