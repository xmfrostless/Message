
#include <Message.h>
#include <iostream>

int main(int, char**) {

    int binder;

    Message::Dispatcher dispatcher;
    dispatcher.AddListener<int>(&binder, [](const int& msg) {
        std::cout << "message: " << msg << std::endl;
    });

    dispatcher.Send(10);

    dispatcher.RemoveListener<int>(&binder);

    dispatcher.Send(10);

    return 0;
}