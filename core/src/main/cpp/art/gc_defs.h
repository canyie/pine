//
// Created by canyie on 2021/5/1.
//

#ifndef PINE_GC_DEFS_H
#define PINE_GC_DEFS_H

namespace pine::art {
    class GCCriticalSection {
    private:
        [[maybe_unused]] void* self_;
        [[maybe_unused]] const char* section_name_;
    };

    enum GcCause {
        // Invalid GC cause used as a placeholder.
        kGcCauseNone [[maybe_unused]],
        // GC triggered by a failed allocation. Thread doing allocation is blocked waiting for GC before
        // retrying allocation.
        kGcCauseForAlloc [[maybe_unused]],
        // A background GC trying to ensure there is free memory ahead of allocations.
        kGcCauseBackground [[maybe_unused]],
        // An explicit System.gc() call.
        kGcCauseExplicit [[maybe_unused]],
        // GC triggered for a native allocation when NativeAllocationGcWatermark is exceeded.
        // (This may be a blocking GC depending on whether we run a non-concurrent collector).
        kGcCauseForNativeAlloc [[maybe_unused]],
        // GC triggered for a collector transition.
        kGcCauseCollectorTransition [[maybe_unused]],
        // Not a real GC cause, used when we disable moving GC (currently for GetPrimitiveArrayCritical).
        kGcCauseDisableMovingGc [[maybe_unused]],
        // Not a real GC cause, used when we trim the heap.
        kGcCauseTrim [[maybe_unused]],
        // Not a real GC cause, used to implement exclusion between GC and instrumentation.
        kGcCauseInstrumentation [[maybe_unused]],
        // Not a real GC cause, used to add or remove app image spaces.
        kGcCauseAddRemoveAppImageSpace [[maybe_unused]],
        // Not a real GC cause, used to implement exclusion between GC and debugger.
        kGcCauseDebugger,
        // GC triggered for background transition when both foreground and background collector are CMS.
        kGcCauseHomogeneousSpaceCompact [[maybe_unused]],
        // Class linker cause, used to guard filling art methods with special values.
        kGcCauseClassLinker [[maybe_unused]],
        // Not a real GC cause, used to implement exclusion between code cache metadata and GC.
        kGcCauseJitCodeCache [[maybe_unused]],
        // Not a real GC cause, used to add or remove system-weak holders.
        kGcCauseAddRemoveSystemWeakHolder [[maybe_unused]],
        // Not a real GC cause, used to prevent hprof running in the middle of GC.
        kGcCauseHprof [[maybe_unused]],
        // Not a real GC cause, used to prevent GetObjectsAllocated running in the middle of GC.
        kGcCauseGetObjectsAllocated [[maybe_unused]],
        // GC cause for the profile saver.
        kGcCauseProfileSaver [[maybe_unused]],
    };

    enum CollectorType {
        // No collector selected.
        kCollectorTypeNone [[maybe_unused]],
        // Non concurrent mark-sweep.
        kCollectorTypeMS [[maybe_unused]],
        // Concurrent mark-sweep.
        kCollectorTypeCMS [[maybe_unused]],
        // Semi-space / mark-sweep hybrid, enables compaction.
        kCollectorTypeSS [[maybe_unused]],
        // Heap trimming collector, doesn't do any actual collecting.
        kCollectorTypeHeapTrim [[maybe_unused]],
        // A (mostly) concurrent copying collector.
        kCollectorTypeCC [[maybe_unused]],
        // The background compaction of the concurrent copying collector.
        kCollectorTypeCCBackground [[maybe_unused]],
        // Instrumentation critical section fake collector.
        kCollectorTypeInstrumentation [[maybe_unused]],
        // Fake collector for adding or removing application image spaces.
        kCollectorTypeAddRemoveAppImageSpace [[maybe_unused]],
        // Fake collector used to implement exclusion between GC and debugger.
        kCollectorTypeDebugger,
        // A homogeneous space compaction collector used in background transition
        // when both foreground and background collector are CMS.
        kCollectorTypeHomogeneousSpaceCompact [[maybe_unused]],
        // Class linker fake collector.
        kCollectorTypeClassLinker [[maybe_unused]],
        // JIT Code cache fake collector.
        kCollectorTypeJitCodeCache [[maybe_unused]],
        // Hprof fake collector.
        kCollectorTypeHprof [[maybe_unused]],
        // Fake collector for installing/removing a system-weak holder.
        kCollectorTypeAddRemoveSystemWeakHolder [[maybe_unused]],
        // Fake collector type for GetObjectsAllocated
        kCollectorTypeGetObjectsAllocated [[maybe_unused]],
        // Fake collector type for ScopedGCCriticalSection
        kCollectorTypeCriticalSection [[maybe_unused]],
    };
}

#endif //PINE_GC_DEFS_H
