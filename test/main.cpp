
#include <Message.h>
#include <iostream>

struct Msg {
    int value { 0 };
};

void Test1() {
    std::cout << std::endl << "------ " << __FUNCTION__ << " ------" << std::endl;
    int binder;
    Message::Dispatcher dispatcher;
    dispatcher.AddListener<int>(&binder, [](auto& msg) {
        std::cout << typeid(msg).name() << ": " << msg << std::endl;
    });
    dispatcher.AddListener<float>(&binder, [](auto& msg) {
        std::cout << typeid(msg).name() << ": " << msg << std::endl;
    });
    dispatcher.AddListener<double>(&binder, [](auto& msg) {
        std::cout << typeid(msg).name() << ": " << msg << std::endl;
    });
    dispatcher.AddListener<std::size_t>(&binder, [](auto& msg) {
        std::cout << typeid(msg).name() << ": " << msg << std::endl;
    });
    dispatcher.AddListener<Msg>(&binder, [](auto& msg) {
        std::cout << typeid(msg).name() << ": " << msg.value << std::endl;
    });
    dispatcher.Send(1);
    dispatcher.Send(2.0f);
    dispatcher.Send(3.0);
    dispatcher.Send(std::size_t(4));
    dispatcher.Send(Msg { 5 });
    dispatcher.RemoveListener<int>(&binder);
    dispatcher.RemoveListener<float>(&binder);
    dispatcher.RemoveListener<double>(&binder);
    dispatcher.RemoveListener<std::size_t>(&binder);
    dispatcher.RemoveListener<Msg>(&binder);
    dispatcher.Send(6);
    dispatcher.Send(7.0f);
    dispatcher.Send(8.0);
    dispatcher.Send(std::size_t(9));
    dispatcher.Send(Msg { 10 });

    dispatcher.AddListener<std::string>(&binder, [](auto& msg) {
        std::cout << typeid(msg).name() << ": " << msg << std::endl;
    });
    dispatcher.AddListener<const char*>(&binder, [](auto& msg) {
        std::cout << typeid(msg).name() << ": " << msg << std::endl;
    });
    dispatcher.Send<std::string>("Hello");
    dispatcher.Send<const char*>("World");
    dispatcher.RemoveAllListeners(&binder);
    dispatcher.Send<std::string>("Good");
    dispatcher.Send<const char*>("Bye");
}

void Test2() {
    std::cout << std::endl << "------ " << __FUNCTION__ << " ------" << std::endl;
    int binder1;
    int binder2;
    int binder3;
    Message::Dispatcher dispatcher;

    dispatcher.AddListener<int>(&binder1, [&](auto& msg1) {
        std::cout << "binder1:" << msg1 << std::endl;
        dispatcher.AddListener<int>(&binder2, [&](auto& msg2) {
            std::cout << "binder2:" << msg2 << std::endl;
            dispatcher.AddListener<int>(&binder3, [&](auto& msg3) {
                std::cout << "binder3:" << msg3 << std::endl;
                dispatcher.RemoveListener<int>(&binder3);
            });
            dispatcher.RemoveListener<int>(&binder2);
        });
        dispatcher.RemoveListener<int>(&binder1);
    });

    dispatcher.Send(10);
    dispatcher.Send(20);
    dispatcher.Send(30);
}

void Test3() {
    std::cout << std::endl << "------ " << __FUNCTION__ << " ------" << std::endl;
    Message::Dispatcher dispatcher;
    dispatcher.AddListener<int>(&dispatcher, [&](auto& msg) {
        std::cout << typeid(msg).name() << ": " << msg << std::endl;
        dispatcher.Send(2.0f);
    });
    dispatcher.AddListener<float>(&dispatcher, [&](auto& msg) {
        std::cout << typeid(msg).name() << ": " << msg << std::endl;
        dispatcher.Send(3.0);
    });
    dispatcher.AddListener<double>(&dispatcher, [&](auto& msg) {
        std::cout << typeid(msg).name() << ": " << msg << std::endl;
        dispatcher.RemoveListener<int>(&dispatcher);
        dispatcher.RemoveListener<float>(&dispatcher);
        dispatcher.RemoveListener<double>(&dispatcher);
    });
    dispatcher.Send(1);
    dispatcher.Send(10);
}

void Test4() {
    std::cout << std::endl << "------ " << __FUNCTION__ << " ------" << std::endl;
    Message::Dispatcher dispatcher;
    dispatcher.AddListener<int>(&dispatcher, [&](auto& msg) {
        dispatcher.RemoveListener<int>(&dispatcher);
        std::cout << typeid(msg).name() << ": " << msg << std::endl;
        dispatcher.Send(2.0f);
    });
    dispatcher.AddListener<float>(&dispatcher, [&](auto& msg) {
        dispatcher.RemoveListener<int>(&dispatcher);
        std::cout << "A: " << typeid(msg).name() << ": " << msg << std::endl;
    });
    dispatcher.AddListener<float>(&dispatcher, [&](auto& msg) {
        std::cout << "B: " << typeid(msg).name() << ": " << msg << std::endl;
    });
    dispatcher.Send(1);
    dispatcher.Send(2);
    dispatcher.Send(3.0f);
    dispatcher.Send(4.0f);
}

void Test5() {
    std::cout << std::endl << "------ " << __FUNCTION__ << " ------" << std::endl;
    Message::Dispatcher dispatcher;
    dispatcher.AddListener<int>(&dispatcher, [&](auto& msg) {
        std::cout << typeid(msg).name() << ": " << msg << " ";
        dispatcher.Send(float(msg + 1));
    });
    dispatcher.AddListener<float>(&dispatcher, [&](auto& msg) {
        if (msg > 500.f) {
            return;
        }
        std::cout << typeid(msg).name() << ": " << msg << " ";
        dispatcher.Send(int(msg + 1));
    });
    dispatcher.Send(0);
    std::cout << std::endl;
}

int main(int, char**) {
    Test1();
    Test2();
    Test3();
    Test4();
    Test5();

    return 0;
}