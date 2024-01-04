#ifndef _INFO_H
#define _INFO_H

typedef struct {
        char name[150];    /*client's FIFO name */
        char password[150];
        char myfifo[150];
        char touser[150];
        char context[150];
}CLIENTINFO, * CLIENTINFOPTR;

#endif
