/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdio.h>
#include <stdlib.h>

void SystemClock_Config(void);

char color = 0;
char firstDigit = 0, secondDigit = 0, thirdDigit = 0;
int gotFirst = 0, gotSecond = 0;
int promptPrinted = 0;

void sendChar(char c)
{
	
	while(!(USART1->ISR & USART_ISR_TXE));
		
	USART1->TDR = c;
}

void sendString(char* str)
{
	
	while(*str)
	{
		sendChar(*str++);
	}
}

int isDigit(char value)
{
	
	if(value == '0' || value == '1' || value == '2' || value == '3' || value == '4' ||
		value == '5' || value == '6' || value == '7' || value == '8' || value == '9')
		
		return 1;
	
	return 0;
}

void error(char* errorMessage)
{
	
	sendString(errorMessage);
	
	promptPrinted = 0;
	gotFirst = 0;
	gotSecond = 0;
	color = 0;
}

void dac()
{
	
	char message[3] = {firstDigit, secondDigit, thirdDigit};
	int value = atoi(message);
	uint8_t val = value;
	
	if(color == 'r')
		DAC1->DHR8R1 = val;
	else
		DAC1->DHR8R2 = val;
}

void USART1_IRQHandler(void)
{
	
	char value = USART1->RDR;
	//sendChar(value);	//So the user sees what they type in terminal; not useful for script
	
	//Get color mode, if we haven't yet.
	if(!color) 
	{
		
		if(value == 'r' || value == 'g')
		{
			color = value;
			sendString("\r\nValue? (0-254)\r\n");
			return;
		}
		
		error("\r\nColor must be 'r' or 'g'\r\n");
		return;
	}
	
	//Get first, second digits
	if(!gotFirst) 
	{
		
		if(isDigit(value))
		{
			firstDigit = value;
			gotFirst = 1;
			
			return;
		}

		error("\r\nPlease enter a digit\r\n");
		return;
	}
	
	if(!gotSecond) 
	{
		
		if(isDigit(value))
		{
			secondDigit = value;
			gotSecond = 1;
			
			return;
		}

		error("\r\nPlease enter a digit\r\n");
		return;
	}
	
	//Grab final digit and feed to dac.
	if(isDigit(value))
	{
		
		thirdDigit = value;
		dac();
		error("\r\n");
			
		return;
	}

	error("\r\nPlease enter a digit\r\n");
}

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();
	
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	
	GPIO_InitTypeDef initStr2 = {GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_6 | GPIO_PIN_7, 
															GPIO_MODE_OUTPUT_PP,
															GPIO_SPEED_FREQ_LOW,
															GPIO_NOPULL};
																
	
	GPIO_InitTypeDef initStr = {GPIO_PIN_6 | GPIO_PIN_7, 
															GPIO_MODE_AF_PP,
															GPIO_SPEED_FREQ_LOW,
															GPIO_NOPULL};
	
	HAL_GPIO_Init(GPIOB, &initStr);
	HAL_GPIO_Init(GPIOC, &initStr2);
 
	GPIOB->AFR[0] &= ~(1 << 24);
	GPIOB->AFR[0] &= ~(1 << 28);
																							
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;	
	USART1->BRR = HAL_RCC_GetHCLKFreq() / 115200;
	USART1->CR1 |= USART_CR1_TE | USART_CR1_RE;		 	
	
	USART1->CR1 |= USART_CR1_RXNEIE;
	NVIC_EnableIRQ(USART1_IRQn);
	NVIC_SetPriority(USART1_IRQn, 3);
															
	USART1->CR1 |= USART_CR1_UE;								
										
	 /* -------------- Now set up DAC! ------------------ */
	RCC->APB1ENR |= RCC_APB1ENR_DACEN;
	
	GPIOA->MODER |= GPIO_MODER_MODER4_0 | GPIO_MODER_MODER5_0;
	
	DAC1->SWTRIGR |= DAC_SWTRIGR_SWTRIG2 | DAC_SWTRIGR_SWTRIG1;

	DAC1->CR |= DAC_CR_EN1 | DAC_CR_EN2;	//Finally, enable														

	while (1)
  {
		
		if(!promptPrinted) {
			
			sendString("Color? (r or g)\r\n");
			promptPrinted = 1;
		}
		
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(char *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
