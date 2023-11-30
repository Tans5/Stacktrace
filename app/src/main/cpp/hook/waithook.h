#ifndef STACKTRACE_WAITHOOK_H
#define STACKTRACE_WAITHOOK_H

typedef struct Message {
    char *chars;
    int charsLen;
} Message;

Message* allocMessage(int size);

void sayHello(Message *msg);

void freeMessage(Message * msg);
#endif //STACKTRACE_WAITHOOK_H
