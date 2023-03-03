#include "dsp.h"

TaskHandle_t dspTaskHandle;

void write_task(void *arg)
{
    StreamBufferHandle_t xStreamBufferRead = (StreamBufferHandle_t)arg;

    while (1)
    {
        /* Receive up to another sizeof( ucRxData ) bytes from the stream buffer.
        Wait in the Blocked state (so not using any CPU processing time) for a
        maximum of 100ms for the full sizeof( ucRxData ) number of bytes to be
        available. */
        xReceivedBytes = xStreamBufferReceive(xStreamBufferRead,
                                              (void *)ucRxData,
                                              READ_BUF_SIZE_BYTES * sizeof(char),
                                              xBlockTime * 2);

        if (xReceivedBytes > 0)
        {
            // do something with the data blocks
        }
        else
        {
            /* For test purposes */
            /* The call to xStreamBufferReceive() timed out before any data was
            available. */
            // color_printf(COLOR_PRINT_RED, "\t\rec_task: notify timeout");
        }
    }
    // delete task
    vTaskDelete(NULL);
}

void init_dsp(StreamBufferHandle_t xStreamBufferRead)
{
    color_printf(COLOR_PRINT_PURPLE, "init_dsp");
    xTaskCreatePinnedToCore(dsp_task, "dsp_task", 2 * CONFIG_SYSTEM_EVENT_TASK_STACK_SIZE, xStreamBufferRead, 5, &dspTaskHandle, 1);
}

void suspend_dsp_task() { vTaskSuspend(dspTaskHandle); }

void resume_dsp_task() { vTaskResume(dspTaskHandle); }