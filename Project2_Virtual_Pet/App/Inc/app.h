#ifndef APP_H
#define APP_H

#include <stdint.h>

void App_Init(uint32_t now_ms);     /* we call it on startup - if it doesn't work it is Mike's fault haha :) */
void App_Update(uint32_t now_ms);   /* call every pass of loop on main */

#endif /* APP_H */