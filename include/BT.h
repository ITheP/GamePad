#pragma once

#include "IconMappings.h"

class BT {
public:
    static char StatusIcon;
    
    static void Init();
    static void RenderIcons();
    static void RenderStatusIcon(char icon);
};