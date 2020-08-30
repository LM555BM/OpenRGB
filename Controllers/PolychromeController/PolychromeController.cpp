/*-----------------------------------------*\
|  PolychromeController.cpp                 |
|                                           |
|  Driver for for ASRock ASR LED and        |
|  Polychrome RGB lighting controller       |
|                                           |
|  Adam Honse (CalcProgrammer1) 12/14/2019  |
\*-----------------------------------------*/

#include "PolychromeController.h"
#include <cstring>

using namespace std::chrono_literals;

PolychromeController::PolychromeController(i2c_smbus_interface* bus, polychrome_dev_id dev)
{
    this->bus = bus;
    this->dev = dev;

    unsigned short fw_version    = ReadFirmwareVersion();
    unsigned char  major_version = fw_version >> 8;
    unsigned char  minor_version = fw_version & 0xFF;

    /*-----------------------------------------------------*\
    | Determine whether the device uses ASR LED or          |
    | Polychrome protocol by checking firmware version.     |
    | Versions 1.xx and 2.xx use ASR LED, 3.xx uses         |
    | Polychrome                                            |
    \*-----------------------------------------------------*/
    if((major_version < 0x03) && (major_version > 0x00))
    {
        device_name = "ASRock ASR LED";
        led_count   = 1;
        asr_led     = true;
    }
    else if(major_version == 0x03)
    {
        device_name = "ASRock Polychrome";
        led_count   = 1;
        asr_led     = false;
    }
    else
    {
        led_count   = 0;
    }
    
}

PolychromeController::~PolychromeController()
{

}

std::string PolychromeController::GetDeviceName()
{
    return(device_name);
}

std::string PolychromeController::GetFirmwareVersion()
{
    unsigned short fw_version    = ReadFirmwareVersion();
    unsigned char  major_version = fw_version >> 8;
    unsigned char  minor_version = fw_version & 0xFF;

    return(std::to_string(major_version) + "." + std::to_string(minor_version));
}

unsigned short PolychromeController::ReadFirmwareVersion()
{
    // The firmware register holds two bytes, so the first read should return 2
    // If not, report invalid firmware revision FFFF
    if (bus->i2c_smbus_read_byte_data(dev, POLYCHROME_REG_FIRMWARE_VER) == 0x02)
    {
        std::this_thread::sleep_for(1ms);
        unsigned char major = bus->i2c_smbus_read_byte(dev);
        std::this_thread::sleep_for(1ms);
        unsigned char minor = bus->i2c_smbus_read_byte(dev);

        return((major << 8) | minor);
    }
    else
    {
        return(0xFFFF);
    }
}

unsigned int PolychromeController::GetLEDCount()
{
    return(led_count);
}

unsigned int PolychromeController::GetMode()
{
    return(active_mode);
}

bool PolychromeController::IsAsrLed()
{
    return(asr_led);
}

void PolychromeController::SetColorsAndSpeed(unsigned char led, unsigned char red, unsigned char green, unsigned char blue)
{
    unsigned char color_speed_pkt[4] = { red, green, blue, active_speed };
    unsigned char select_led_pkt[1]  = { led };
    
    if (asr_led)
    {
        switch(active_mode)
        {
            /*-----------------------------------------------------*\
            | These modes take 4 bytes in R/G/B/S order             |
            \*-----------------------------------------------------*/
            case ASRLED_MODE_BREATHING:
            case ASRLED_MODE_FLASHING:
            case ASRLED_MODE_SPECTRUM_CYCLE:
                bus->i2c_smbus_write_block_data(dev, active_mode, 4, color_speed_pkt);
                break;

            /*-----------------------------------------------------*\
            | These modes take 3 bytes in R/G/B order               |
            \*-----------------------------------------------------*/
            default:
            case ASRLED_MODE_STATIC:
            case ASRLED_MODE_MUSIC:
                bus->i2c_smbus_write_block_data(dev, active_mode, 3, color_speed_pkt);
                break;

            /*-----------------------------------------------------*\
            | These modes take 1 byte - speed                       |
            \*-----------------------------------------------------*/
            case ASRLED_MODE_RANDOM:
            case ASRLED_MODE_WAVE:
                bus->i2c_smbus_write_block_data(dev, active_mode, 1, &active_speed);
                break;

            /*-----------------------------------------------------*\
            | These modes take no bytes                             |
            \*-----------------------------------------------------*/
            case ASRLED_MODE_OFF:
                break;
        }
        std::this_thread::sleep_for(1ms);
    }
    else
    {
        /*-----------------------------------------------------*\
        | Select LED                                            |
        \*-----------------------------------------------------*/
        bus->i2c_smbus_write_block_data(dev, POLYCHROME_REG_LED_SELECT, 1, select_led_pkt);

        /*-----------------------------------------------------*\
        | Polychrome firmware always writes color to fixed reg  |
        \*-----------------------------------------------------*/
        bus->i2c_smbus_write_block_data(dev, POLYCHROME_REG_COLOR, 3, color_speed_pkt);

        std::this_thread::sleep_for(1ms);
    }
}

void PolychromeController::SetMode(unsigned char mode, unsigned char speed)
{
    unsigned char led_count_pkt[1]  = { 0x00 };
    active_mode                     = mode;
    active_speed                    = speed;

    if(asr_led)
    {
        bus->i2c_smbus_write_block_data(dev, ASRLED_REG_MODE, 1, &active_mode);
        std::this_thread::sleep_for(1ms);
    }
    else
    {
        bus->i2c_smbus_write_block_data(dev, POLYCHROME_REG_MODE, 1, &active_mode);
        std::this_thread::sleep_for(1ms);

        /*-----------------------------------------------------*\
        | Select a single LED                                   |
        \*-----------------------------------------------------*/
        bus->i2c_smbus_write_block_data(dev, POLYCHROME_REG_LED_COUNT, 0, led_count_pkt);
        std::this_thread::sleep_for(1ms);

        switch(active_mode)
        {
            /*-----------------------------------------------------*\
            | These modes don't take a speed                        |
            \*-----------------------------------------------------*/
            case POLYCHROME_MODE_OFF:
            case POLYCHROME_MODE_STATIC:
                break;

            /*-----------------------------------------------------*\
            | All other modes, write speed to active mode register  |
            \*-----------------------------------------------------*/
            default:
                bus->i2c_smbus_write_block_data(dev, active_mode, 1, &speed);
                std::this_thread::sleep_for(1ms);
                break;
        }
    }    
}
