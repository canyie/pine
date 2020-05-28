//
// Created by canyie on 2020/3/19.
//

#ifndef PINE_ARM32_H
#define PINE_ARM32_H

#include <cstdint>
#include "../trampoline_installer.h"
#include "../../utils/macros.h"

namespace pine {
    class Arm32TrampolineInstaller final : public TrampolineInstaller {
    protected:
        virtual void InitTrampolines() override ;
        virtual bool CannotBackup(art::ArtMethod* target) override ;

    private:
        static bool IsPCRelatedInst(uint32_t inst);
    };
}


#endif //PINE_ARM32_H
