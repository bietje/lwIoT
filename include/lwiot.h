/*
 * lwIoT initialisation header.
 *
 * @author Michel Megens
 * @email  dev@bietje.net
 */

#pragma once

#include <lwiot/lwiot.h>

#ifdef __cplusplus
extern "C" void lwiot_init();
extern "C" void lwiot_destroy();
#else
extern void lwiot_init();
extern void lwiot_destroy();
#endif

