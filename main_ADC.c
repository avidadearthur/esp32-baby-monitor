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
#include "stm8s_adc1.h"
#include "math.h";

#define A 0.003354016
#define B 0.0002569850
#define C 0.000002620131
#define D 0.0000000638309
#define R_REF 2200
#define V_in 5

uint16_t adc_value;
float temperature_Kelvin;
float temperature_Celsius;
float R_NTC;
float V_out;


void main(void)
{
	GPIO_Init(GPIOB, GPIO_PIN_0, GPIO_MODE_IN_FL_NO_IT);
	
	ADC1_DeInit();
	ADC1_Init(	ADC1_CONVERSIONMODE_CONTINUOUS,
									ADC1_CHANNEL_0, 
									ADC1_PRESSEL_FCPU_D18, 
									ADC1_EXTTRIG_GPIO,
									ENABLE,
									ADC1_ALIGN_RIGHT,
									ADC1_SCHMITTTRIG_CHANNEL0,
									DISABLE
								);
	ADC1_Cmd(ENABLE);
	ADC1_ScanModeCmd(DISABLE);
	ADC1_DataBufferCmd(DISABLE);
	ADC1_StartConversion();

  while (1)
  {
		adc_value = ADC1_GetConversionValue();
		V_out = V_in*adc_value/(float)1024;
		R_NTC = (V_out*R_REF)/(V_in-V_out);
		temperature_Kelvin = 1/((A+B*log(R_NTC/R_REF))+(C*pow(log(R_NTC/R_REF),2))+(D*pow(log(R_NTC/R_REF),3)));
		temperature_Celsius = temperature_Kelvin - 273.15;
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
