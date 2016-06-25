#ifndef DINO_H
#define DINO_H

// This header is a wrapper that conditionally either includes DINO headers or
// stubs its public API. This wrapper exists to be able to build with and
// without DINO without having to edit the application source. Note that
// obviously, this functionality cannot be encapsulated into libdino, because
// the point of building without DINO is to not depend on libdino. But, we keep
// this header in the libdino repo, and let each app copy it as needed.

#ifdef DINO

#include <libdino/dino.h>

#else // !DINO

#define DINO_RESTORE_CHECK()
#define DINO_TASK_BOUNDARY(...)

#define DINO_MANUAL_VERSION_PTR(...)
#define DINO_MANUAL_VERSION_VAL(...)
#define DINO_MANUAL_RESTORE_NONE()
#define DINO_MANUAL_RESTORE_PTR(...)
#define DINO_MANUAL_RESTORE_VAL(...)
#define DINO_MANUAL_REVERT_BEGIN(...)
#define DINO_MANUAL_REVERT_END(...)
#define DINO_MANUAL_REVERT_VAL(...)

#endif // !DINO

#endif // DINO_H
