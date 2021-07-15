/*
 * imu.c
 *
 *  Created on: July 15, 2021
 *      Author: Benson Yang
 */

#include "imu.h"

static int16_t Burst_Cmd = 0x6800;

void ADIS16467_Init(ADIS16467_t *device)
{
    ADI_Write_Reg(device, GLOB_CMD_REG, 0x80); //software reset
    ADI_Write_Reg(device, GLOB_CMD_REG + 1, 0x80);
    HAL_Delay(400); // wait for reboot

    ADI_Write_Reg(device, FILT_CTRL_REG, 0x03); //set Bartlett filter level
    ADI_Write_Reg(device, FILT_CTRL_REG + 1, 0x00);

    ADI_Write_Reg(device, DEC_RATE_REG, 0x03); //set value of filter to 4
    ADI_Write_Reg(device, DEC_RATE_REG + 1, 0x00);
}

void ADIS16467_Read_Accel(ADIS16467_t *device)
{
    uint16_t data[2];

    ADI_Read_Reg(device, X_ACCL_LOW_REG, data, 2);
    device->Ax = (int32_t)((data[1] << 16) & 0xFFFF0000 | data[0]) * (0.00125 / (1 << 16));

    ADI_Read_Reg(device, Y_ACCL_LOW_REG, data, 2);
    device->Ay = (int32_t)((data[1] << 16) & 0xFFFF0000 | data[0]) * (0.00125 / (1 << 16));

    ADI_Read_Reg(device, Z_ACCL_LOW_REG, data, 2);
    device->Az = (int32_t)((data[1] << 16) & 0xFFFF0000 | data[0]) * (0.00125 / (1 << 16));
}

void ADIS16467_Read_Gyro(ADIS16467_t *device)
{
    uint16_t data[2];

    ADI_Read_Reg(device, X_GYRO_LOW_REG, data, 2);
    device->Gx = (int32_t)((data[1] << 16) & 0xFFFF0000 | data[0]) / 655360.0f;

    ADI_Read_Reg(device, Y_GYRO_LOW_REG, data, 2);
    device->Gy = (int32_t)((data[1] << 16) & 0xFFFF0000 | data[0]) / 655360.0f;

    ADI_Read_Reg(device, Z_GYRO_LOW_REG, data, 2);
    device->Gz = (int32_t)((data[1] << 16) & 0xFFFF0000 | data[0]) / 655360.0f;
}

void ADIS16467_Read_Temp(ADIS16467_t *device)
{
    uint16_t data[1];
    ADI_Read_Reg(device, TEMP_OUT_REG, data, 1);
    device->Temperature = (int16_t)data * 0.1;
}

int8_t ADI_Read_Reg(ADIS16467_t *device, uint8_t addr, uint16_t *receive, uint8_t num)
{
    uint16_t Tx_tmp = 0, Rx_tmp = 0;

    //first frame only transmit
    Tx_tmp = addr << 8;
    Rx_tmp = ADI_flame_TandR(device, Tx_tmp);
    for(uint8_t i=1; i<num; i++)
    {
        Tx_tmp = (addr + 2 * i) << 8;
        Rx_tmp = ADI_flame_TandR(device, Tx_tmp);
        receive[i-1] = Rx_tmp;
    }
    //last frame only receive
    Tx_tmp = 0;
    receive[num-1] = ADI_flame_TandR(device, Tx_tmp);

    return 0;
}
int8_t ADI_Write_Reg(ADIS16467_t *device, uint8_t addr, uint8_t value)
{
    addr |= 0x80; //写数据的掩码
    uint16_t Tx_tmp = (addr << 8) | value;
    ADI_flame_TandR(device, Tx_tmp);
    return 0;
}
uint16_t ADI_flame_TandR(ADIS16467_t *device, uint16_t trans)
{
    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 0);
    uint16_t result;
    static HAL_StatusTypeDef state;
    state = HAL_SPI_TransmitReceive(device->hspi, (uint8_t *)&trans, (uint8_t *)&result, 1, 0xFFFF);
    if (state != HAL_OK)
    {
        while (1)
            ;
    }
    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 1);
    sb_delay(150);
    return result;
}
void sb_delay(volatile uint32_t t)
{
    while (t--)
        ;
}

/*
void ADIS16467_Init(ADIS16467_t *device)
{
    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 0);
    HAL_SPI_TransmitReceive(device->hspi, RANG_MDL_REG, &(device->rangeModel), 1, 0xFFFF);
    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 1);
    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 0);
    HAL_SPI_TransmitReceive(device->hspi, PROD_ID_REG, &(device->prodId), 1, 0xFFFF);
    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 1);
}

void ADIS16467_Read_Accel(ADIS16467_t *device)
{
    uint16_t low, high;
    HAL_StatusTypeDef res;
    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 0);
    HAL_SPI_TransmitReceive(device->hspi, X_ACCL_LOW_REG, &low, 1, 0xFFFF);
    HAL_SPI_TransmitReceive(device->hspi, X_ACCL_OUT_REG, &high, 1, 0xFFFF);
    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 1);
    device->Ax = (int32_t)((high << 16) & 0xFFFF0000 | low) * (0.00125 / (1 << 16));

    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 0);
    HAL_SPI_TransmitReceive(device->hspi, Y_ACCL_LOW_REG, &low, 1, 0xFFFF);
    HAL_SPI_TransmitReceive(device->hspi, Y_ACCL_OUT_REG, &high, 1, 0xFFFF);
    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 1);
    device->Ay = (int32_t)((high << 16) & 0xFFFF0000 | low) * (0.00125 / (1 << 16));

    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 0);
    HAL_SPI_TransmitReceive(device->hspi, Z_ACCL_LOW_REG, &low, 1, 0xFFFF);
    HAL_SPI_TransmitReceive(device->hspi, Z_ACCL_OUT_REG, &high, 1, 0xFFFF);
    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 1);
    device->Az = (int32_t)((high << 16) & 0xFFFF0000 | low) * (0.00125 / (1 << 16));
}

void ADIS16467_Read_Gyro(ADIS16467_t *device)
{
    double defScale = 1.0;
    defScale = (model == 0x3) ? (0.00625 / (1 << 16)) : (model == 0x7) ? (0.025 / (1 << 16)) : (model == 0xF)   ? (0.1 / (1 << 16)) : defScale;

    uint16_t low, high;
    HAL_StatusTypeDef res;
    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 0);
    HAL_SPI_TransmitReceive(device->hspi, X_ACCL_LOW_REG, &low, 1, 0xFFFF);
    HAL_SPI_TransmitReceive(device->hspi, X_ACCL_OUT_REG, &high, 1, 0xFFFF);
    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 1);
    device->Ax = (int32_t)((high << 16) & 0xFFFF0000 | low) * defScale;

    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 0);
    HAL_SPI_TransmitReceive(device->hspi, Y_ACCL_LOW_REG, &low, 1, 0xFFFF);
    HAL_SPI_TransmitReceive(device->hspi, Y_ACCL_OUT_REG, &high, 1, 0xFFFF);
    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 1);
    device->Ay = (int32_t)((high << 16) & 0xFFFF0000 | low) * defScale;

    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 0);
    HAL_SPI_TransmitReceive(device->hspi, Z_ACCL_LOW_REG, &low, 1, 0xFFFF);
    HAL_SPI_TransmitReceive(device->hspi, Z_ACCL_OUT_REG, &high, 1, 0xFFFF);
    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 1);
    device->Az = (int32_t)((high << 16) & 0xFFFF0000 | low) * defScale;
}

void ADIS16467_Read_Temp(ADIS16467_t *device)
{
    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 0);
    HAL_SPI_TransmitReceive(device->hspi, TEMP_OUT_REG, &(device->prodId), 1, 0xFFFF);
    HAL_GPIO_WritePin(device->GPIOx, device->GPIO_PIN, 1);
}
*/