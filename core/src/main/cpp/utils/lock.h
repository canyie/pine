//
// Created by canyie on 2020/3/11.
//

#ifndef PINE_LOCK_H
#define PINE_LOCK_H
namespace pine {
    class ScopedLock {
    public:
        inline ScopedLock(std::mutex& mutex) : mLock(mutex)  { mLock.lock(); }
        inline ScopedLock(std::mutex* mutex) : mLock(*mutex) { mLock.lock(); }
        inline ~ScopedLock() { mLock.unlock(); }
    private:
        std::mutex& mLock;
    };
}
#endif //PINE_LOCK_H
