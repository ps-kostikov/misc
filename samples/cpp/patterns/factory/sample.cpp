#include <iostream>
#include <memory>

class Storage;

class StorageFactory {
public:
    StorageFactory() {}
    std::unique_ptr<Storage> getStorage();
};

class Session {
public:
    Session(Storage* storage): storage_(storage) {}

    void action();

private:
    std::unique_ptr<Storage> storage_;
};

class Storage {
public:
    Storage() {}
    void update() {
        std::cout << "storage::update" << std::endl;
    }
};

std::unique_ptr<Storage> StorageFactory::getStorage() {
    return std::unique_ptr<Storage>(new Storage);
}

void Session::action() {
    std::cout << "session::action" << std::endl;
    storage_->update();
}

int main()
{
    std::cout << "hello world" << std::endl;
    StorageFactory factory;
    auto storage = factory.getStorage();
    Session session(storage.release());
    session.action();
    return 0;
}
