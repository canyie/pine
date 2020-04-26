//
// Created by canyie on 2020/4/7.
//

#ifndef PINE_ARM64_H
#define PINE_ARM64_H

#include <cstdint>
#include "../trampoline_installer.h"

namespace pine {
    class Arm64TrampolineInstaller final : public TrampolineInstaller {
    protected:
        virtual void InitTrampolines() override ;
        virtual bool CannotBackup(art::ArtMethod *target) override ;

    private:
        static bool IsPCRelatedInst(uint32_t inst);
    };
}

#endif //PINE_ARM64_H
