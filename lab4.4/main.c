/******************************************************************************
* File Name:   main.c
*
* Description: lab 4.4 --> Distance measurement over bluetooth
*
* Sources Used:
* 	https://infineon.github.io/bless/ble_api_reference_manual/html/page_ble_quick_start.html#group_ble_quick_start1
* 	https://community.infineon.com/t5/Blogs/Creating-a-Bluetooth-LE-Peripheral-using-ModusToolbox-2/ba-p/247094
*	https://community.infineon.com/t5/Blogs/Creating-a-Bluetooth-LE-Peripheral-using-ModusToolbox-4/ba-p/247099
*
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
#include "cy_syslib.h"
#include "cy_sysint.h"
#include "cycfg.h"
#include "cy_ble_gatt.h"
#include "cycfg_ble.h"

#define SOUND_SPEED_CMUS 0.0343 // speed of sound in centimeters per microsecond


// Struts and globals
cy_stc_sysint_t echoConfig =
{
		.intrSrc = ioss_interrupts_gpio_9_IRQn
};

const cy_stc_sysint_t blessIsrCfg =
{
     .intrSrc      = bless_interrupt_IRQn,
     .intrPriority = 1u
};

static cy_stc_ble_gatt_handle_value_pair_t sendData = {
	.value.val = NULL,
	.value.len = 1,
	.attrHandle = CY_BLE_CUSTOM_SERVICE_CUSTOM_CHARACTERISTIC_CHAR_HANDLE
};

static uint8_t distance_byte = 0;

static cy_stc_ble_conn_handle_t conn_handle;

static int firstRead = 1; // 1 indicates that it is our first read, 0 indicates that it is not our first read


// Interrupt functions
static void BlessInterrupt(void)
{
    /* Call interrupt processing */
    Cy_BLE_BlessIsrHandler();
}

void AppCallBack( uint32_t event, void* eventParam )
{
    if (event == CY_BLE_EVT_STACK_ON) { // bluetooth stack enabled, start advertising
    	printf("BLE stack on\r\n");
		Cy_BLE_GAPP_StartAdvertisement( CY_BLE_ADVERTISING_FAST, CY_BLE_PERIPHERAL_CONFIGURATION_0_INDEX );
    }
    else if (event == CY_BLE_EVT_GATT_CONNECT_IND) { // device connected
    	printf("Connected\r\n");
    	conn_handle = *(cy_stc_ble_conn_handle_t *)eventParam;
    }
    else if (event == CY_BLE_EVT_GAP_DEVICE_DISCONNECTED) { // device disconnected, restart advertising
    	printf("Disconnected\r\n");
		Cy_BLE_GAPP_StartAdvertisement( CY_BLE_ADVERTISING_FAST, CY_BLE_PERIPHERAL_CONFIGURATION_0_INDEX);
		conn_handle.attId = 0;
		conn_handle.bdHandle = 0;
    }
    else if (event == CY_BLE_EVT_GATTS_READ_CHAR_VAL_ACCESS_REQ ) { // client wants to read value, send trigger
    	if (firstRead) {
    		firstRead = 0;
    		return;
    	}
    	sendData.value.val = &distance_byte; // update data
    	printf("New Read\r\n");
    	printf("Decimal: %d\r\n", distance_byte);
    	printf("Hex:     %x\r\n\r\n", distance_byte);

		// send the struct
		if (Cy_BLE_GATTS_WriteAttributeValueLocal(&sendData) != CY_BLE_GATT_ERR_NONE) {
			printf("Error occurred writing value\r\n");
		}
	}
    return;
}


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

    	// get distance into sendData struct
		if ( Cy_BLE_GetConnectionState( conn_handle ) == CY_BLE_CONN_STATE_CONNECTED ) {
			if (distance < 256) {
				distance_byte = (uint8_t)distance; // get distance as a byte
			}
		}

    	Cy_GPIO_ClearInterrupt(ECHO_PORT, ECHO_NUM); // clear interrupt

		// send next trigger
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
    __enable_irq();
    setvbuf( stdin, NULL, _IONBF, 0 );
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);



    // BLUETOOTH INITIALIZATION
    // initialize bless interrupt
	cy_ble_config.hw->blessIsrConfig = &blessIsrCfg;
	Cy_SysInt_Init(cy_ble_config.hw->blessIsrConfig, BlessInterrupt);

	// initialize callback function for generic events
	Cy_BLE_RegisterEventCallback(AppCallBack);

	// initialize and enable
	Cy_BLE_Init(&cy_ble_config);
	Cy_BLE_EnableLowPowerMode();
	Cy_BLE_Enable();


	// SENSOR INITIALIZATION
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
		Cy_BLE_ProcessEvents();
	}
}

/* [] END OF FILE */

