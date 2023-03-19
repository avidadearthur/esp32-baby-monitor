/**
  ******************************************************************************
  * @file    Project/main.c 
  * @author  MCD Application Team
  * @version V2.3.0
  * @date    16-June-2017
  * @brief   Main program body
   ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2014 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 


#include "stm8s.h"

void main(void)
{

	// Hall Sensor
	
	GPIO_Init(GPIOD, GPIO_PIN_3, GPIO_MODE_OUT_PP_LOW_FAST);
	GPIO_Init(GPIOB, GPIO_PIN_1, GPIO_MODE_IN_FL_NO_IT);
	
	// NTC - High Temperature Threshold
	
	GPIO_Init(GPIOA, GPIO_PIN_1, GPIO_MODE_OUT_PP_LOW_FAST);
	GPIO_Init(GPIOF, GPIO_PIN_4, GPIO_MODE_IN_FL_NO_IT);
	
	// NTC - Low Temperature Threshold
	
	GPIO_Init(GPIOA, GPIO_PIN_2, GPIO_MODE_OUT_PP_LOW_FAST);
	GPIO_Init(GPIOD, GPIO_PIN_0, GPIO_MODE_IN_FL_NO_IT);
	

  while (1)
  {
		

		// Reading the Hall sensor
		// If changing magnetic field is detected, then external LED D3 is ON.
		
		if(GPIO_ReadInputPin(GPIOB, GPIO_PIN_1)){
			GPIO_WriteHigh(GPIOD, GPIO_PIN_3);}
		else{
			GPIO_WriteLow(GPIOD, GPIO_PIN_3);}
		
		// Reading the NTC - checks if temperature is too high
		// If too high, then LED A2 is ON
		
		if(GPIO_ReadInputPin(GPIOF, GPIO_PIN_4)){
			GPIO_WriteHigh(GPIOA, GPIO_PIN_2);}
		else{
			GPIO_WriteLow(GPIOA, GPIO_PIN_2);}
			
		// Reading the NTC - checks if temperature is too low
		// If too low, then LED A1 is ON
		
		if(GPIO_ReadInputPin(GPIOD, GPIO_PIN_0)){
			GPIO_WriteHigh(GPIOA, GPIO_PIN_1);}
		else{
			GPIO_WriteLow(GPIOA, GPIO_PIN_1);}
  }
}


#ifdef USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval : None
  */
void assert_failed(u8* file, u32 line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
