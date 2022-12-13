# What is this about ?
This is an example code from a tutorial about inter-task communication. I thought this was useful because we'll probably deal with this kind of architecture to send ADC data to the nRF24 and from the nRF24 to the DAC. The video can be found [here](https://www.youtube.com/watch?v=DLqj01asDM0) and it describes a scenario where we have two tasks filling a queue that is being emptied by a receiver task. I had to remove some parts because of deprecated libraries.

# Background theory
From Kolban's book on esp32 (it's already shared in the OneDrive):
> ## The architecture of a task in FreeRTOS
> Let us start with the notion of a task. A task is a piece of work that we wish to perform. 
> If you wish, you can think of this as a C language function. For example:
>```
> int add(int a, int b) {
>  return a + b;
> }
>```
> could be considered a task … although this would be ridiculously simple. Generically, 
> think of a task as the execution of a piece of C code that you have authored. We 
> normally think of code running from its start all the way through to its end … however, 
> this is not necessarily the most efficient way to proceed. Consider the idea of an 
> application which wishes to send some data over the network. It may wish to send a 
> megabyte of data … however it may also find that it can only send 100K at a time 
> before it has to wait for the transmitted data to be delivered. In that story, it would send
> 100K and wait for the transmission to complete, send the next 100K and wait for that 
> transmission to complete and so on. But what of those periods of time where the code 
> is waiting for a previous transmission to complete? What is the CPU doing at those 
> times?
> The chances are that it is doing nothing but monitoring the flag that states that the 
> transmission has completed. This is a waste. In theory the CPU could be performing 
> other work (assuming that there is in fact other work that could be performed). If there 
> is indeed other work available, we could "context switch" between these work items 
> such that when one blocks waiting for something to happen, control could be passed to 
> another to do something useful.
> If we call each piece of work "a task", that is the value of a task in FreeRTOS. The task 
> represents a piece of work to be performed but instead of assuming that the work will 
> quickly go from start to end, we are declaring that there may be times within the work 
> where it can relinquish control to other work (tasks).
> This can be illustrated pictorially in the following. First Task A is running and then it 
> either blocks or else is preempted and Task B runs. It runs for a bit and then there is a 
> context switch back to Task A and finally, Task C gets control. At any one time, an 
> individual core is only ever running one task but because of the context switching, we 
> achieve the effect that over some measured time period, ALL the tasks ran.
> With this in mind, we should think about how a task is created. There is an API 
> provided by FreeRTOS called "xTaskCreate()" which creates an instance of a task.

_See the book and the FreeRTOS API docs for more info about `xTaskCreate()`._

> ##Queues within RTOS
> A queue is a data container abstraction. Think of a queue as holding items. When an 
> item is added to a queue, it is added to the end. When an item is removed from a 
> queue, it is removed from the front. This provides a first-in/first-out paradigm. Think of 
> a queue as the line of folks waiting at the department of motor vehicles. Folks join the 
> end of the queue and those that have arrived earlier are serviced earlier.
> In FreeRTOS queues, all items on the queue are placed there by copy and not by 
> reference.
> When a queue is created, the maximum number of items it can hold is set and fixed at 
> that time. The storage for the queue is also allocated. This means that we can't run out
> of storage for the queues when adding new items.
> In RTOS we call xQueueSendToBack() to add an entry at the end of the queue. The 
> consumer calls xQueueReceive() to retrieve the person at the head of the queue. Items 
> in the queue are always stored by copy rather than by reference.
> Attempt to receive from an empty queue can cause the caller to block. Conversely, 
> attempting to add an item into a full queue can also cause it to block until space is freed.
> When we attempt to read an item from the queue and there is no item to read, the call 
> to read will block. We can specify a timeout value measured in ticks. If no new 
> message arrives in that interval, we will return with an indication that no message was 
> retrieved. We can also specify 0 to return immediately if there are no messages. While 
> a task is blocked reading from a queue that task is placed in the blocked state meaning 
> that other tasks can run. This means that we won't be blocking the system while waiting
> on a queue. When a message arrives, the task that was previously blocked is returned 
> to the ready state and becomes eligible to be executed by the task scheduler.





