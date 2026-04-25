# include "timer.h"
# include "type.h"
# include "uart.h"
# include "string.h"
# include "fdt.h"
# include "task.h"
# include "sbi.h"

unsigned long CLOCK_FREQ;

static struct timer_event* time_head = NULL;
// record time event's print info
struct timeout_info {
    char msg[64];
    unsigned long start_time;   // when instruction exec time
};

unsigned long get_time(){
    unsigned long n;
    asm volatile("rdtime %0" : "=r"(n));
    return n;
}

void periodic_boot_time_task(void *arg) {
    unsigned long now_tick = get_time();
    unsigned long boot_seconds = now_tick / CLOCK_FREQ;

    uart_puts("boot time: ");
    uart_putd((unsigned int)boot_seconds);
    uart_puts("\n");

    add_timer(periodic_boot_time_task, NULL, 2);
}

void timer_init(){
    add_timer(periodic_boot_time_task, NULL, 2);
}

void fdt_timer_init(const void* fdt){
    int cpus_offset = fdt_path_offset(fdt, "/cpus");
    if(cpus_offset >= 0){
        int len;
        uint32_t* prop = (uint32_t*)fdt_getprop(fdt, cpus_offset, "timebase-frequency", &len);
        if(prop){
            CLOCK_FREQ = bswap32(*prop);
            // uart_puts("[Kernel] DETECTED Timer Frequency: ");
            // uart_putd(CLOCK_FREQ);
            // uart_puts(" Hz\n");
        }
    }
}

// add time event into line
void add_timer(void (*callback)(void*), void* arg, int sec){
    /*if(callback == NULL)
        return;*/
    struct timer_event* new_event = (struct timer_event*)allocate(sizeof(struct timer_event));
    if(!new_event)
        return;

    new_event->expire_time = get_time() + (unsigned long)sec * CLOCK_FREQ;
    new_event->callback = callback;
    new_event->arg = arg;
    new_event->next = NULL;

    // disable global interrupt(sstatus.SIE)
    //asm volatile("csrci sstatus, 1 << 1");

    if(time_head == NULL || (new_event->expire_time < time_head->expire_time)){
        new_event->next = time_head;
        time_head = new_event;
        sbi_set_timer(new_event->expire_time);  // Update earliest time
    }else{
        struct timer_event* curr = time_head;
        while(curr->next != NULL && (curr->next->expire_time < new_event->expire_time)){
            curr = curr->next;
        }
        new_event->next = curr->next;
        curr->next = new_event;
    }

    // enable global interrupt(sstatus.SIE)
    //asm volatile("csrsi sstatus, 1 << 1");
}

// execute time event's print out
void timeout_callback(void* arg){
    struct timeout_info *info = (struct timeout_info *)arg;
    unsigned long now = get_time();
    uart_puts(" Command Executed Time: ");
    uart_putd(info->start_time / CLOCK_FREQ);
    uart_puts(" s /");
    uart_puts(" Current Time: ");
    uart_putd(now / CLOCK_FREQ);
    uart_puts(" s / ");
    uart_puts(info->msg);
    uart_puts("\n");
    free(info);
}

// prepare parameters for add_timer
void do_setTimeout(char* sec_ptr, char* msg_ptr){
    // calculate sec
    int sec = 0;
    while(*sec_ptr >= '0' && *sec_ptr <= '9'){
        sec = sec * 10 + (*sec_ptr - '0');
        sec_ptr++;
    }

    struct timeout_info *info = (struct timeout_info *)allocate(sizeof(struct timeout_info));
    int i = 0;
    while(msg_ptr[i] != '\0' && i < 63){
        info->msg[i] = msg_ptr[i];
        i++;
    }
    info->msg[i] = '\0';
    info->start_time = get_time();
    add_timer(timeout_callback, (void*)info, sec);
}

void timer_event_handler(){
    unsigned long now = get_time();

    while(time_head != NULL && time_head->expire_time <= now){
        struct timer_event* event = time_head;
        time_head = time_head->next;
        add_task((task_callback_t)event->callback, event->arg, 5);

        free(event);
    }

    if(time_head != NULL){
        sbi_set_timer(time_head->expire_time);
    }else{
        sbi_set_timer(-1ULL);
    }
}