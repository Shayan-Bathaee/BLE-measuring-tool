/******************************************************************************
* File Name:   main.c
*
* Description: lab 4.2 --> efficient measuring tool
*
* Related Document: See README.md
*
*
*******************************************************************************
* (c) 2019-2021, Cypress Semiconductor Corporation. All rights reserved.
*******************************************************************************
* This software, including source code, documentation and related materials
* ("Software"), is owned by Cypress Semiconductor Corporation or one of its
* subsidiaries ("Cypress") and is protected by and subject to worldwide patent
* protection (United States and foreign), United States copyright laws and
* international treaty provisions. Therefore, you may use this Software only
* as provided in the license agreement accompanying the software package from
* which you obtained this Software ("EULA").
*
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software source
* code solely for use in connection with Cypress's integrated circuit products.
* Any reproduction, modification, translation, compilation, or representation
* of this Software except as specified above is prohibited without the express
* written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer of such
* system or application assumes all risk of such use and in doing so agrees to
* indemnify Cypress against all liability.
*******************************************************************************/

#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "DFR0554.h"
#include <stdio.h>
#include <string.h>

#define SOUND_SPEED_CMUS 0.0343 // speed of sound in centimeters per microsecond

cy_stc_sysint_t echoConfig =
{
		.intrSrc = ioss_interrupts_gpio_9_IRQn
};

void ECHO_HANDLER(){
	uint32_t echo = Cy_GPIO_Read(ECHO_PORT, ECHO_NUM);
	if (echo == 1) {
		Cy_TCPWM_TriggerStart(TIMER_HW, TIMER_MASK); // start timer
		Cy_GPIO_SetInterruptEdge(ECHO_PORT, ECHO_NUM, CY_GPIO_INTR_FALLING); // set interrupt to falling edge
		Cy_GPIO_ClearInterrupt(ECHO_PORT, ECHO_NUM); // clear interrupt
		return;
	}
	else {
		// get distance
		uint32_t time = Cy_TCPWM_Counter_GetCounter(TIMER_HW, TIMER_NUM);
		uint32_t distance = (time * SOUND_SPEED_CMUS) / 2;

		Cy_GPIO_SetInterruptEdge(ECHO_PORT, ECHO_NUM, CY_GPIO_INTR_RISING); // set interrupt to rising edge

		// reset counter
		Cy_TCPWM_TriggerStopOrKill(TIMER_HW, TIMER_MASK);
		Cy_TCPWM_Counter_SetCounter(TIMER_HW, TIMER_NUM, 0);

    	// write distance to the screen
		char buf[16];
    	LCD_SetCursor(0, 0);
    	sprintf(buf, "     %3lu cm     ", distance);
    	LCD_Print(buf);

    	Cy_GPIO_ClearInterrupt(ECHO_PORT, ECHO_NUM); // clear interrupt

    	// trigger
    	Cy_GPIO_Set(TRIGGER_PORT, TRIGGER_NUM);
		CyDelayUs(10);
		Cy_GPIO_Clr(TRIGGER_PORT, TRIGGER_NUM);
		return;
	}
}

int main(void)
{
    cy_rslt_t result;

    /* Initialize the device and board peripherals */
    result = cybsp_init() ;
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    // retarget io stuff
    setvbuf( stdin, NULL, _IONBF, 0 );
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

    __enable_irq();

    // initialize scb
    cy_stc_scb_i2c_context_t I2C_context;
    Cy_SCB_I2C_Init(I2C_HW, &I2C_config, &I2C_context);
    Cy_SCB_I2C_Enable(I2C_HW);
    LCD_Start(I2C_HW, &I2C_context);

    // initialize timer
    // every timer count represents 1 microsecond
    if (CY_TCPWM_SUCCESS != Cy_TCPWM_Counter_Init(TIMER_HW, TIMER_NUM, &TIMER_config)) {
    	printf("Timer failed\r\n");
    }
    Cy_TCPWM_Counter_Enable(TIMER_HW, TIMER_NUM);

    // set up echo pin interrupts
    Cy_SysInt_Init(&echoConfig, ECHO_HANDLER);
    NVIC_ClearPendingIRQ(echoConfig.intrSrc);
    Cy_GPIO_SetInterruptMask(ECHO_PORT, ECHO_NUM, 1UL);
    NVIC_EnableIRQ(echoConfig.intrSrc);

    // send first trigger
	Cy_GPIO_Set(TRIGGER_PORT, TRIGGER_NUM);
	CyDelayUs(10);
	Cy_GPIO_Clr(TRIGGER_PORT, TRIGGER_NUM);


    for (;;)
    {
    }
}

/* [] END OF FILE */
