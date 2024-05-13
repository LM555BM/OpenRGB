/*-----------------------------------------*\
|  RGBController_LEDStrip.cpp               |
|                                           |
|  Generic RGB Interface for OpenAuraSDK    |
|  KeyboardVisualizer LED strip interface   |
|                                           |
|  Adam Honse (CalcProgrammer1) 6/20/2019   |
\*-----------------------------------------*/

#include "RGBController_LEDStrip.h"

/**------------------------------------------------------------------*\
    @name Serial LED Strip
    @category LEDStrip
    @type Serial
    @save :x:
    @direct :white_check_mark:
    @effects :x:
    @detectors DetectLEDStripControllers
    @comment
\*-------------------------------------------------------------------*/

RGBController_LEDStrip::RGBController_LEDStrip(LEDStripController* controller_ptr)
{
    controller  = controller_ptr;

    name        = "LED Strip";
    type        = DEVICE_TYPE_LEDSTRIP;
    description = "Serial LED Strip Device";
    location    = controller->GetLocation();

    if(controller->GetLEDKind() == RGB_LED_STRIP)
    {
        mode Direct;
        Direct.name       = "Direct";
        Direct.value      = 0;
        Direct.flags      = MODE_FLAG_HAS_PER_LED_COLOR;

        if(controller->GetProtocol() == LED_PROTOCOL_TPM2_MODIFIED)
        {
            Direct.flags |= MODE_FLAG_MANUAL_SAVE;
        }

        Direct.color_mode = MODE_COLORS_PER_LED;
        modes.push_back(Direct);
    }
    else if(controller->GetLEDKind() == RGBW_LED_STRIP)
    {
        mode Direct;
        Direct.name       = "Direct";
        Direct.value      = 0;
        Direct.flags      = MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_MANUAL_SAVE | MODE_FLAG_HAS_BRIGHTNESS;

        Direct.color_mode = MODE_COLORS_PER_LED;
        Direct.brightness_min = 0;
        Direct.brightness_max = 255;
        Direct.temprature_min = 0;
        Direct.temprature_max = 255;
        modes.push_back(Direct);
    }

    else if(controller->GetLEDKind() == RGBCCT_LED_STRIP)
    {
        mode Direct;
        Direct.name       = "Direct";
        Direct.value      = 0;
        Direct.flags      = MODE_FLAG_HAS_PER_LED_COLOR | MODE_FLAG_MANUAL_SAVE | MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_TEMPRATURE;

        Direct.color_mode = MODE_COLORS_PER_LED;
        Direct.brightness_min = 0;
        Direct.brightness_max = 255;
        Direct.temprature_min = 0;
        Direct.temprature_max = 255;
        modes.push_back(Direct);
    }


    SetupZones();
}

RGBController_LEDStrip::~RGBController_LEDStrip()
{
    delete controller;
}

void RGBController_LEDStrip::SetupZones()
{
    zone led_zone;
    led_zone.name       = "LED Strip";
    led_zone.type       = ZONE_TYPE_LINEAR;
    led_zone.leds_min   = controller->num_leds;
    led_zone.leds_max   = controller->num_leds;
    led_zone.leds_count = controller->num_leds;
    led_zone.matrix_map = NULL;
    zones.push_back(led_zone);

    for(int led_idx = 0; led_idx < controller->num_leds; led_idx++)
    {
        led new_led;
        new_led.name    = "LED ";
        new_led.name.append(std::to_string(led_idx));

        leds.push_back(new_led);
    }

    SetupColors();
}

void RGBController_LEDStrip::ResizeZone(int /*zone*/, int /*new_size*/)
{
    /*---------------------------------------------------------*\
    | This device does not support resizing zones               |
    \*---------------------------------------------------------*/
}

void RGBController_LEDStrip::DeviceUpdateLEDs()
{
    //Evlt. reduzierbar da atribute immer vorhanden
    /*
    if(!(this->modes[active_mode].flags &= MODE_FLAG_HAS_BRIGHTNESS))
    {
        controller->SetLEDs(colors);
    }
    else if((this->modes[active_mode].flags &= (MODE_FLAG_HAS_BRIGHTNESS | MODE_FLAG_HAS_TEMPRATURE)))
    {*/
        controller->SetLEDs(colors,this->modes[active_mode].brightness,this->modes[active_mode].temprature);
    //}

}

void RGBController_LEDStrip::UpdateZoneLEDs(int /*zone*/)
{
    controller->SetLEDs(colors);
}

void RGBController_LEDStrip::UpdateSingleLED(int /*led*/)
{
    controller->SetLEDs(colors);
}

void RGBController_LEDStrip::DeviceUpdateMode()
{

}

void RGBController_LEDStrip::DeviceSaveMode()
{
    //0x01 is the TPM2 controll sequence for saving the configuration (Only available with TMP2 Modified)
    std::vector<GiveCommands> saveCommand;
    saveCommand.push_back(0x01);
    controller->WriteTPM2Modified(saveCommand);

}
