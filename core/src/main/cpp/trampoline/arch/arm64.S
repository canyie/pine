#define FUNCTION(name) \
.data; \
.align 4; \
.global name; \
name:

#define VAR(name) \
.global name; \
name:

FUNCTION(pine_direct_jump_trampoline)
ldr x17, pine_direct_jump_trampoline_jump_entry
br x17
VAR(pine_direct_jump_trampoline_jump_entry)
.long 0
.long 0

FUNCTION(pine_bridge_jump_trampoline)
ldr x17, pine_bridge_jump_trampoline_target_method
cmp x0, x17
bne pine_bridge_jump_trampoline_call_origin_entry
ldr x17, pine_bridge_jump_trampoline_extras
b acquire_lock

lock_failed:
wfe // Wait other thread to release the lock

acquire_lock:
ldaxr w16, [x17]
cbz w16, lock_failed // lock_flag == 0 (has other thread holding the lock), fail.
stlxr w16, wzr, [x17] // try set lock_flag to 0
cbnz w16, lock_failed // failed, try again.

// Now we hold the lock!
str x1, [x17, #4]
str x2, [x17, #12]
str x3, [x17, #20]
mov x1, x0 // first param = callee ArtMethod
mov x2, x17 // second param = extras (saved x1, x2, x3)
mov x3, sp // third param = sp
ldr x0, pine_bridge_jump_trampoline_bridge_method
ldr x17, pine_bridge_jump_trampoline_bridge_entry
br x17
VAR(pine_bridge_jump_trampoline_target_method)
.long 0
.long 0
VAR(pine_bridge_jump_trampoline_extras)
.long 0
.long 0
VAR(pine_bridge_jump_trampoline_bridge_method)
.long 0
.long 0
VAR(pine_bridge_jump_trampoline_bridge_entry)
.long 0
.long 0
VAR(pine_bridge_jump_trampoline_call_origin_entry)
.long 0
.long 0

FUNCTION(pine_call_origin_trampoline)
ldr x0, pine_call_origin_trampoline_origin_method
ldr x17, pine_call_origin_trampoline_origin_code_entry
br x17
VAR(pine_call_origin_trampoline_origin_method)
.long 0
.long 0
VAR(pine_call_origin_trampoline_origin_code_entry)
.long 0
.long 0

FUNCTION(pine_backup_trampoline)
ldr x0, pine_backup_trampoline_origin_method
VAR(pine_backup_trampoline_override_space)
.long 0 // 4 bytes (will be overwritten)
.long 0 // 4 bytes (will be overwritten)
.long 0 // 4 bytes (will be overwritten)
.long 0 // 4 bytes (will be overwritten)
ldr x17, pine_backup_trampoline_remaining_code_entry
br x17
VAR(pine_backup_trampoline_origin_method)
.long 0
.long 0
VAR(pine_backup_trampoline_remaining_code_entry)
.long 0
.long 0

FUNCTION(pine_trampolines_end)
// End tag used for calculate size only
