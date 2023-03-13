#include "color_out.h"
#include "transport.h"
#include "audio.h"
#include "sdRecording.h"

static const char *TAG = "espnow_test";

/* When passed to init_recording xStreamBuffer to handle the recording. This
functionality is just for testing purposes. */
static StreamBufferHandle_t network_stream_buf;
static StreamBufferHandle_t recording_stream_buf;

TaskHandle_t senderTaskHandle = NULL;
TaskHandle_t receiverTaskHandle = NULL;

static uint8_t broadcast_mac[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
static uint8_t dst_mac_address[6] = {0xC0, 0x49, 0xEF, 0xCD, 0xE8, 0x4C};

/**
 * @brief WiFi should start before using ESPNOW.
 * ESPNOW can has long range mode but it is not used here.
 */
static void espnow_wifi_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(ESPNOW_WIFI_MODE));
    ESP_ERROR_CHECK(esp_wifi_start());
}

/**
 * @brief Callback function that is executed when data is received.
 */
static void receiver_callback(const uint8_t *mac_addr, const uint8_t *data, int len)
{
    if (mac_addr == NULL || data == NULL || len <= 0)
    {
        ESP_LOGE(TAG, "Receive cb arg error");
        return;
    }

    // params of espnow_recv_task is recieved from esp_now_send(mac_addr, buffer, len)
    if (xStreamBufferSend(network_stream_buf, data, len, portMAX_DELAY) != len)
    {
        ESP_LOGE(TAG, "Failed to send data to network stream buffer: %d", errno);
    }
}

/**
 * @brief Initialize ESP-NOW. (Complete doc stub later.)
 */
static void init_esp_now()
{
    esp_err_t err = esp_now_init();
    if (err != ESP_OK)
    {
        printf("Error starting ESP NOW\n");
    }

    /* Register Callback. */
    err = esp_now_register_recv_cb(receiver_callback);
    if (err != ESP_OK)
    {
        printf("Error registering ESP NOW receiver callback\n");
    }

    /* Set primary master key. */
    ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK));

    /* Add peer information to peer list. */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL)
    {
        printf("Malloc peer information fail");
        /* TODO: (Future Work) Delete Stream Buffer. */
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, dst_mac_address, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(peer));
    free(peer);
}

/**
 * @brief (Complete doc stub later.)
 */
static void init_non_volatile_storage()
{
    esp_err_t err = nvs_flash_init();

    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }

    if (err != ESP_OK)
    {
        printf("Error initializing NVS\n");
    }
}

void sender_task(void *arg)
{
    /* xStreamBuffer to Receive data from audio capture task (tx_task) */
    uint8_t *ucRxData = (uint8_t *)calloc(READ_BUF_SIZE_BYTES, sizeof(char));
    size_t xReceivedBytes;
    const TickType_t xBlockTime = pdMS_TO_TICKS(20);

    StreamBufferHandle_t mic_stream_buf = (StreamBufferHandle_t)arg;

    color_printf(COLOR_PRINT_GREEN, "\t\tsender_task: starting to listen");

    while (1)
    {
        /* Receive up to another sizeof( ucRxData ) bytes from the stream buffer.
        Wait in the Blocked state (so not using any CPU processing time) for a
        maximum of 100ms for the full sizeof( ucRxData ) number of bytes to be
        available. */
        xReceivedBytes = xStreamBufferReceive(mic_stream_buf,
                                              (void *)ucRxData,
                                              READ_BUF_SIZE_BYTES * sizeof(char),
                                              xBlockTime);

        if (xReceivedBytes > 0)
        {
            /* Send data. */
            esp_err_t err = esp_now_send(dst_mac_address, ucRxData, 250);
            if (err != ESP_OK)
            {
                printf("Error sending ESP NOW packet: %x\n", err);
            }
        }
    }
    // delete task after recording
    vTaskDelete(NULL);
}

void init_sender(StreamBufferHandle_t xStreamBufferMic)
{
    esp_err_t err = esp_now_init();
    if (err != ESP_OK)
    {
        printf("Error starting ESP NOW\n");
    }

    /* Set primary master key. */
    ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK));

    /* Add peer information to peer list. */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL)
    {
        printf("Malloc peer information fail");
        /* todo: (Future Work) Delete Stream Buffer. */
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    memcpy(peer->peer_addr, dst_mac_address, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(peer));
    free(peer);

    network_stream_buf = NULL;
    recording_stream_buf = NULL;

    xTaskCreate(sender_task, "sender_task", 2 * CONFIG_SYSTEM_EVENT_TASK_STACK_SIZE, xStreamBufferMic, 10, &senderTaskHandle);
}

void suspend_sender()
{
    vTaskSuspend(senderTaskHandle);
}

void resume_sender()
{
    vTaskResume(senderTaskHandle);
}

void init_receiver(StreamBufferHandle_t xStreamBufferNet)
{
    esp_err_t err = esp_now_init();
    if (err != ESP_OK)
    {
        printf("Error starting ESP NOW\n");
    }

    /* Register Callback. */
    err = esp_now_register_recv_cb(receiver_callback);
    if (err != ESP_OK)
    {
        printf("Error registering ESP NOW receiver callback\n");
    }

    /* Set primary master key. */
    ESP_ERROR_CHECK(esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK));

    /* Add peer information to peer list. */
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    if (peer == NULL)
    {
        printf("Malloc peer information fail");
        /* todo: (Future Work) Delete Stream Buffer. */
    }
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESPNOW_WIFI_IF;
    peer->encrypt = false;
    // todo: change to specific mac address
    memcpy(peer->peer_addr, broadcast_mac, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK(esp_now_add_peer(peer));
    free(peer);

    network_stream_buf = xStreamBufferNet;
    recording_stream_buf = NULL;
}

void init_transport()
{
    /* Initialize NVS. */
    init_non_volatile_storage();

    /* Initialize WiFi. */
    espnow_wifi_init();

    /* Get MAC address (Arthur's board: C0:49:EF:CD:C7:48). */
    uint8_t src_mac_address[6];
    esp_wifi_get_mac(ESP_IF_WIFI_STA, src_mac_address);

    printf("SRC ESP MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
           src_mac_address[0], src_mac_address[1], src_mac_address[2],
           src_mac_address[3], src_mac_address[4], src_mac_address[5]);

    /* Print destination MAC address. */
    printf("DST ESP MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
           dst_mac_address[0], dst_mac_address[1], dst_mac_address[2],
           dst_mac_address[3], dst_mac_address[4], dst_mac_address[5]);
}