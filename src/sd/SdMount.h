/**
 * Shared SD mount for AppSD browse + SdLogger (CoreS3 SPI pins).
 * Ref-counted so leaving AppSD does not kill an active log file.
 */
#ifndef SD_MOUNT_H
#define SD_MOUNT_H

#include <Arduino.h>

bool SdCardPresent();
bool SdMount();   /* idempotent; ref++ */
void SdUnmount(); /* ref--; SD.end only at 0 */
bool SdIsMounted();

#endif
