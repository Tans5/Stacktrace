#include <cstdlib>
#include "waithook.h"

Message* allocMessage(int size) {
    auto *msg = static_cast<Message *>(malloc(sizeof(Message)));
    char *chars = static_cast<char *>(malloc(size));
    msg->chars = chars;
    msg->charsLen = size;
    return msg;
}

void freeMessage(Message* msg) {
    char* chars = msg->chars;
    msg->chars = nullptr;
    free(chars);
    free(msg);
}

void sayHello(Message* msg) {
    sprintf(msg->chars, "Hello, World!!");
}