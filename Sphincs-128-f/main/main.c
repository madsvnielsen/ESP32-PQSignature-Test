#include <stdio.h>
#include <stdint.h>
#include "api.h" // SPHINCSSHAKE128FSIMPLE API header
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_task_wdt.h"
#include "esp_timer.h" 
#include "esp_heap_caps.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "esp_netif.h"
#define MAX_PACKET_SIZE 10000
#define LISTEN_PORT 8000
#define TAG "WIFI_APP"
#define MEASURE_WIFI 0

void measure_task(void *param)
{
    // KEY GENERATION
    uint8_t *public_key = (uint8_t *)malloc(PQCLEAN_SPHINCSSHAKE128FSIMPLE_CLEAN_CRYPTO_PUBLICKEYBYTES);
    uint8_t *private_key = (uint8_t *)malloc(PQCLEAN_SPHINCSSHAKE128FSIMPLE_CLEAN_CRYPTO_SECRETKEYBYTES);

    if (!public_key || !private_key)
    {
        if (public_key)
            free(public_key);
        if (private_key)
            free(private_key);
        return;
    }

    int64_t start_us = esp_timer_get_time();
    // Generate keypair
    if (PQCLEAN_SPHINCSSHAKE128FSIMPLE_CLEAN_crypto_sign_keypair(public_key, private_key) != 0)
    {
        free(public_key);
        free(private_key);
        return;
    }
    int64_t end_us = esp_timer_get_time();
    printf("%lld,", end_us - start_us);

    // SIGNING
    uint8_t message[] = "1234567890123"; // Message to be signed
    uint8_t *signature = malloc(PQCLEAN_SPHINCSSHAKE128FSIMPLE_CLEAN_CRYPTO_BYTES);
    if (signature == NULL)
    {
        printf("Failed to allocate memory for signature\n");
        vTaskDelete(NULL);
    }
    size_t signature_len;

    start_us = esp_timer_get_time();

    if (PQCLEAN_SPHINCSSHAKE128FSIMPLE_CLEAN_crypto_sign_signature(
            signature, &signature_len, message, sizeof(message), private_key) != 0)
    {
        printf("Signing failed\n");
        free(signature);
        vTaskDelete(NULL);
    }

    end_us = esp_timer_get_time();
    printf("%lld,", end_us - start_us);

    // VERIFICATION
    start_us = esp_timer_get_time();

    if (PQCLEAN_SPHINCSSHAKE128FSIMPLE_CLEAN_crypto_sign_verify(
            signature, signature_len, message, sizeof(message), public_key) != 0)
    {
        printf("Verification failed\n");
        return;
    }
    end_us = esp_timer_get_time();

    printf("%lld\n", end_us - start_us);
    free(public_key);
    free(private_key);
    free(signature);
    printf("total_heap: %d\n", heap_caps_get_total_size(MALLOC_CAP_DEFAULT));
    printf("min_free_heap: %ld\n", esp_get_minimum_free_heap_size());
    vTaskDelete(NULL);
}

static void wait_for_ip_address()
{
    esp_netif_ip_info_t ip_info;
    while (true)
    {
        if (esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_STA_DEF"), &ip_info) == ESP_OK)
        {
            if (ip_info.ip.addr != 0)
            {
                ESP_LOGI(TAG, "IP Address: " IPSTR, IP2STR(&ip_info.ip));
                break;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void listen_and_sign_task(void *param)
{
    // NVS INIT
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    // WiFI Config
    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_netif_create_default_wifi_sta();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "SOME_SSID",
            .password = "SOME_PASSWORD",
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_ERROR_CHECK(esp_wifi_connect());

    wait_for_ip_address();

    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (listen_sock < 0)
    {
        ESP_LOGE(TAG, "Socket creation failed\n");
        vTaskDelete(NULL);
        return;
    }

    struct sockaddr_in server_addr = {
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_family = AF_INET,
        .sin_port = htons(LISTEN_PORT),
    };

    if (bind(listen_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        ESP_LOGE(TAG, "Socket bind failed\n");
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    if (listen(listen_sock, 1) < 0)
    {
        ESP_LOGE(TAG, "Socket listen failed\n");
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Listening on port %d\n", LISTEN_PORT);

    // GENERATE KEYPAIR
    uint8_t *public_key = (uint8_t *)malloc(PQCLEAN_SPHINCSSHAKE128FSIMPLE_CLEAN_CRYPTO_PUBLICKEYBYTES);
    uint8_t *private_key = (uint8_t *)malloc(PQCLEAN_SPHINCSSHAKE128FSIMPLE_CLEAN_CRYPTO_SECRETKEYBYTES);
    if (!public_key || !private_key)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for keys\n");
        free(public_key);
        free(private_key);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    if (PQCLEAN_SPHINCSSHAKE128FSIMPLE_CLEAN_crypto_sign_keypair(public_key, private_key) != 0)
    {
        ESP_LOGE(TAG, "Key generation failed\n");
        free(public_key);
        free(private_key);
        close(listen_sock);
        vTaskDelete(NULL);
        return;
    }

    // Keep signing recieved packets
    while (1)
    {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_sock < 0)
        {
            ESP_LOGE(TAG, "Socket accept failed\n");
            continue;
        }

        ESP_LOGI(TAG, "Client connected\n");

        uint8_t incoming_data[MAX_PACKET_SIZE];
        int len = recv(client_sock, incoming_data, MAX_PACKET_SIZE, 0);
        if (len > 0)
        {
            ESP_LOGI(TAG, "Received %d bytes\n", len);
            // SIGNING
            uint8_t *signature = malloc(PQCLEAN_SPHINCSSHAKE128FSIMPLE_CLEAN_CRYPTO_BYTES);
            if (signature)
            {
                size_t signature_len;
                if (PQCLEAN_SPHINCSSHAKE128FSIMPLE_CLEAN_crypto_sign_signature(
                        signature, &signature_len, incoming_data, len, private_key) == 0)
                {
                    send(client_sock, signature, signature_len, 0);
                }
                else
                {
                    ESP_LOGE(TAG, "Signing failed\n");
                }
                free(signature);
            }
        }

        close(client_sock);
        ESP_LOGI(TAG, "Client disconnected\n");
    }

    free(public_key);
    free(private_key);
    close(listen_sock);
    vTaskDelete(NULL);
}

void app_main(void)
{
    if (MEASURE_WIFI)
    {
        xTaskCreate(listen_and_sign_task, "ListenTask", 8192, NULL, 5, NULL);
    }
    else
    {
        for (;;)
        {
            xTaskCreate(measure_task, "MeasureTask", 8192, NULL, 5, NULL);
        }
    }
}
