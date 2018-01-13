#include <rtthread.h>
#include <board.h>
#include <dfs_fs.h>
#include <dfs_posix.h>
#include <dfs_ramfs.h>
#define USB_MSG_READ_FILE       0x00
#define USB_MSG_WIRTE_FILE      0x01
#define USB_MSG_GET_FILE_SIZE   0x02
struct usb_cmd_message
{
    rt_uint8_t message_type;
    rt_uint32_t length;
    char file[128];
};
typedef struct usb_cmd_message * usb_cmd_msg_t;
static struct rt_completion rx_completion,tx_completion;
static struct rt_messagequeue usb_mq;
static rt_uint8_t usb_mq_pool[(sizeof(struct usb_cmd_message)+sizeof(void*))*4];
static struct rt_thread usb_thread;
ALIGN(RT_ALIGN_SIZE)
static char usb_thread_stack[0x1000];

static void usb_thread_entry(void * parameter)
{
    struct usb_cmd_message msg;
    FILE * fp;
    rt_uint8_t * pbuffer;
    struct stat statbuf;
    while(1)
    {
        if(rt_mq_recv(&usb_mq, &msg, sizeof(struct usb_cmd_message), RT_WAITING_FOREVER) != RT_EOK )
            continue;
        switch(msg.message_type)
        {
        case USB_MSG_GET_FILE_SIZE:
            stat(msg.file,&statbuf);
            rt_completion_init(&tx_completion);
            rt_device_write((rt_device_t)parameter,0,&statbuf.st_size,msg.length);
            rt_completion_wait(&tx_completion,RT_WAITING_FOREVER);
            break;
        case USB_MSG_READ_FILE:
            pbuffer = (rt_uint8_t *)rt_malloc(msg.length);
            RT_ASSERT(pbuffer != RT_NULL);
            fp = fopen(msg.file, "r");
            RT_ASSERT(fp != RT_NULL);
            fread(pbuffer, 1, msg.length, fp);
            rt_completion_init(&tx_completion);
            rt_device_write((rt_device_t)parameter,0,pbuffer,msg.length);
            rt_completion_wait(&tx_completion,RT_WAITING_FOREVER);
            rt_free(pbuffer);
            pbuffer = RT_NULL;
            fclose(fp);
            break;
        case USB_MSG_WIRTE_FILE:
            if(msg.length == 0)
            {
                dfs_file_unlink(msg.file);
            }
            else
            {
                pbuffer = (rt_uint8_t *)rt_malloc(msg.length);
                fp = fopen(msg.file,"w");
                RT_ASSERT(fp != RT_NULL);
                rt_completion_init(&rx_completion);
                rt_device_read((rt_device_t)parameter,0,pbuffer,msg.length);
                rt_completion_wait(&rx_completion,RT_WAITING_FOREVER);
                fwrite(pbuffer,1,msg.length,fp);
                fclose(fp);
                rt_free(pbuffer);
                pbuffer = RT_NULL;
            }
        }

    }
}
static void cmd_handler(rt_uint8_t *buffer,rt_size_t size)
{
    struct usb_cmd_message msg;
    buffer[size] = '\0';
    msg.message_type = *buffer;
    msg.length = *((rt_uint32_t *)(buffer+1));
    strcpy((char *)msg.file,(const char *)buffer+5);
    rt_mq_send(&usb_mq, (void*)&msg, sizeof(struct usb_cmd_message));
}

static rt_err_t rx_indicate(rt_device_t dev, rt_size_t size)
{
    rt_completion_done(&rx_completion);
    return RT_EOK;
}
static rt_err_t tx_complete(rt_device_t dev, void *buffer)
{
    rt_completion_done(&tx_completion);
    return RT_EOK;
}
static int application_usb_init(void)
{
    rt_device_t device = rt_device_find("winusb");
    RT_ASSERT(device != RT_NULL);
    rt_device_control(device,RT_DEVICE_CTRL_CONFIG,(void *)cmd_handler);
    device->rx_indicate = rx_indicate;
    device->tx_complete = tx_complete;
    rt_device_open(device,RT_DEVICE_FLAG_RDWR);
    /* create an usb message queue */
    rt_mq_init(&usb_mq,
               "winusb",
               usb_mq_pool, sizeof(struct usb_cmd_message),
               sizeof(usb_mq_pool),
               RT_IPC_FLAG_FIFO);

    /* init usb device thread */
    rt_thread_init(&usb_thread,
                   "winusb",
                   usb_thread_entry, device,
                   usb_thread_stack, sizeof(usb_thread_stack),
                   6, 20);
    return rt_thread_startup(&usb_thread);

}
INIT_APP_EXPORT(application_usb_init);



