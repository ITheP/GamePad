#include "BT.h"
#include <Icons.h>
#include "UI.h"
#include "IconMappings.h"

char BT::StatusIcon = Icon_EyesLeft;
    

void BT::Init()
{
      //StatusIcon = Icon_EyesLeft;
}

void BT::RenderIcons()
{
    RenderIcon(Icon_BTLogo, uiBT_xPos, uiBT_yPos, 0, 0);
    RenderIcon(StatusIcon, uiBTStatus_xPos, uiBTStatus_yPos, 16, 12);
}

void BT::RenderStatusIcon(char icon)
{
    StatusIcon = icon;
    RenderIcon(icon, uiBTStatus_xPos, uiBTStatus_yPos, 16, 12);
}